#include "zip_writer.hpp"
#include "../common/safe_file_writer.hpp"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSet>
#include <QtEndian>
#include <array>
#include <limits>
namespace photo26::core {
namespace {
void le16(QByteArray& b,quint16 n) { char p[2];qToLittleEndian(n,p);b.append(p,2); }
void le32(QByteArray& b,quint32 n) { char p[4];qToLittleEndian(n,p);b.append(p,4); }
quint32 crcUpdate(quint32 crc,const QByteArray& b) {
    static const auto table=[] { std::array<quint32,256> t{}; for(quint32 i=0;i<256;++i) { quint32 c=i;for(int j=0;j<8;++j)c=(c>>1)^((c&1)?0xedb88320u:0);t[i]=c;}return t;}();
    for(unsigned char c:b) crc=table[(crc^c)&255]^(crc>>8); return crc;
}
struct Item { QString name; const QByteArray* bytes{}; QString path; };
StatusResult zip(const QString& target,const std::vector<Item>& items,bool overwrite,const CancellationToken& cancel) {
    if(items.size()>65535) return StatusResult::failure(ErrorCode::InvalidParameter,"ZIP clásico admite como máximo 65535 entradas");
    QSet<QString> names;
    for(const auto& i:items) { QString n=i.name; n.replace('\\','/'); const auto parts=n.split('/'); if(n.isEmpty() || n.startsWith('/') || n.contains(':') || n.contains(QChar(0)) || parts.contains("..") || parts.contains(".") || parts.contains("") || n.toUtf8().size()>65535 || names.contains(n)) return StatusResult::failure(ErrorCode::InvalidParameter,"Nombre ZIP inseguro, duplicado o demasiado largo",n); names.insert(n); }
    return SafeFileWriter::writeWithCallback(target,[&](const QString& tmp) {
        QFile out(tmp); if(!out.open(QIODevice::WriteOnly|QIODevice::Truncate)) return StatusResult::failure(ErrorCode::FileWriteError,"No se pudo abrir ZIP",out.errorString()); QByteArray central;
        auto write=[&](const QByteArray& b){return out.write(b)==b.size();};
        for(const auto& i:items) {
            if(cancel.isCancelled()) return StatusResult::failure(ErrorCode::Cancelled,"Creación ZIP cancelada");
            QFile in(i.path); if(!i.bytes && !in.open(QIODevice::ReadOnly)) return StatusResult::failure(ErrorCode::FileReadError,"No se pudo abrir un archivo del ZIP",i.path);
            const qint64 size=i.bytes?i.bytes->size():in.size(),offset=out.pos();
            if(size<0 || quint64(size)>0xffffffffULL || quint64(offset)>0xffffffffULL) return StatusResult::failure(ErrorCode::InvalidParameter,"Se requiere ZIP64 para archivos de más de 4 GiB");
            QString norm=i.name;norm.replace('\\','/');QByteArray name=norm.toUtf8(),h;
            // UTF-8 filename flag (0x0800), Store method (no lossy transformation, bounded streaming memory).
            // Local file header CRC-32 and sizes are patched in-place after streaming for maximum compatibility.
            le32(h,0x04034b50);le16(h,20);le16(h,0x0800);le16(h,0);le16(h,0);le16(h,33);le32(h,0);le32(h,quint32(size));le32(h,quint32(size));le16(h,quint16(name.size()));le16(h,0);h+=name;
            if(!write(h)) return StatusResult::failure(ErrorCode::FileWriteError,"Error escribiendo ZIP");
            quint32 crc=0xffffffff; qint64 done=0;
            while(done<size) { if(cancel.isCancelled()) return StatusResult::failure(ErrorCode::Cancelled,"Creación ZIP cancelada"); QByteArray b=i.bytes?i.bytes->mid(done,qMin<qint64>(1024*1024,size-done)):in.read(qMin<qint64>(1024*1024,size-done)); if(b.isEmpty() || !write(b)) return StatusResult::failure(ErrorCode::FileWriteError,"Error leyendo o escribiendo entrada ZIP",i.name);crc=crcUpdate(crc,b);done+=b.size(); }
            if(!i.bytes && in.size()!=size) return StatusResult::failure(ErrorCode::ConflictDetected,"El archivo cambió durante la creación del ZIP",i.path);
            crc^=0xffffffff;
            const qint64 endPos=out.pos();
            QByteArray patch; le32(patch,crc); le32(patch,quint32(size)); le32(patch,quint32(size));
            if(!out.seek(offset+14)||!write(patch)||!out.seek(endPos)) return StatusResult::failure(ErrorCode::FileWriteError,"Error actualizando cabecera ZIP");
            le32(central,0x02014b50);le16(central,20);le16(central,20);le16(central,0x0800);le16(central,0);le16(central,0);le16(central,33);le32(central,crc);le32(central,quint32(size));le32(central,quint32(size));le16(central,quint16(name.size()));le16(central,0);le16(central,0);le16(central,0);le16(central,0);le32(central,0);le32(central,quint32(offset));central+=name;
        }
        if(cancel.isCancelled()) return StatusResult::failure(ErrorCode::Cancelled,"Creación ZIP cancelada");
        if(quint64(out.pos())+quint64(central.size())+22>0xffffffffULL) return StatusResult::failure(ErrorCode::InvalidParameter,"El archivo requiere ZIP64");
        quint32 offset=quint32(out.pos());if(!write(central)) return StatusResult::failure(ErrorCode::FileWriteError,"Error escribiendo directorio ZIP");QByteArray end;le32(end,0x06054b50);le16(end,0);le16(end,0);le16(end,quint16(items.size()));le16(end,quint16(items.size()));le32(end,quint32(central.size()));le32(end,offset);le16(end,0);
        if(!write(end)||!out.flush()) return StatusResult::failure(ErrorCode::FileWriteError,"Error finalizando ZIP");return StatusResult::success();
    },{overwrite,true});
}
}
StatusResult ZipWriter::createZipFromEntries(const QString& path,const std::vector<ZipEntry>& entries,bool overwrite) { std::vector<Item> items;for(const auto& e:entries)items.push_back({e.archivePath,&e.data,{}});return zip(path,items,overwrite,{}); }
StatusResult ZipWriter::createZipFromFiles(const QString& path,const std::vector<std::pair<QString,QString>>& files,bool overwrite,const CancellationToken& cancel) { std::vector<Item> items;for(const auto& [disk,name]:files)items.push_back({name,nullptr,disk});return zip(path,items,overwrite,cancel); }
}
