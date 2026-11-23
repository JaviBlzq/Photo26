#include "ico_codec.hpp"
#include "../common/safe_file_writer.hpp"
#include <QBuffer>
#include <QColorSpace>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtEndian>
#include <set>
#include <limits>
namespace photo26::core {
namespace {
void u16(QByteArray& b, quint16 n) { char s[2]; qToLittleEndian(n,s); b.append(s,2); }
void u32(QByteArray& b, quint32 n) { char s[4]; qToLittleEndian(n,s); b.append(s,4); }
QImage renderIcon(const QImage& src,int size,const QColor& bg,IconFitMode mode,int padding) {
    QImage out(size,size,QImage::Format_ARGB32); out.fill(bg); out.setColorSpace(src.colorSpace());
    const int inset=size*padding/100, inner=size-2*inset;
    QImage scaled=src.scaled(inner,inner,mode==IconFitMode::Stretch?Qt::IgnoreAspectRatio:(mode==IconFitMode::CoverCrop?Qt::KeepAspectRatioByExpanding:Qt::KeepAspectRatio),Qt::SmoothTransformation);
    QPainter p(&out); p.setClipRect(inset,inset,inner,inner); p.drawImage((size-scaled.width())/2,(size-scaled.height())/2,scaled); return out;
}
Result<QByteArray> pngBytes(const QImage& image) { QByteArray bytes; QBuffer buf(&bytes); buf.open(QIODevice::WriteOnly); if(!image.save(&buf,"PNG")) return Result<QByteArray>::failure(ErrorCode::FileWriteError,"No se pudo codificar PNG"); return Result<QByteArray>::success(bytes); }
}
Result<QByteArray> IcoCodec::encodeMultiResolutionIco(const QImage& source,const std::vector<int>& sizes,const QColor& bg,IconFitMode fit,int pad) {
    if(source.isNull() || sizes.empty() || sizes.size()>65535 || pad<0 || pad>49) return Result<QByteArray>::failure(ErrorCode::InvalidParameter,"Imagen, tamaños o margen de icono inválidos");
    std::vector<QByteArray> payloads;
    for(int size:sizes) { if(size<1 || size>256) return Result<QByteArray>::failure(ErrorCode::InvalidParameter,"ICO admite tamaños de 1 a 256 píxeles"); auto png=pngBytes(renderIcon(source,size,bg,fit,pad)); if(!png.isOk()) return png; payloads.push_back(std::move(*png.value)); }
    QByteArray out; u16(out,0); u16(out,1); u16(out,quint16(sizes.size())); quint64 offset=6+16*sizes.size();
    for(size_t i=0;i<sizes.size();++i) { const int n=sizes[i]; out.append(char(n==256?0:n)); out.append(char(n==256?0:n)); out.append(char(0)); out.append(char(0)); u16(out,1); u16(out,32); u32(out,quint32(payloads[i].size())); u32(out,quint32(offset)); offset+=payloads[i].size(); if(offset>std::numeric_limits<quint32>::max()) return Result<QByteArray>::failure(ErrorCode::InvalidParameter,"ICO demasiado grande"); }
    for(const auto& p:payloads) out+=p;
    return Result<QByteArray>::success(out);
}
StatusResult IcoCodec::saveMultiResolutionIco(const QImage& src,const QString& path,const std::vector<int>& sizes,const QColor& bg,IconFitMode fit,int pad,bool overwrite) { auto r=encodeMultiResolutionIco(src,sizes,bg,fit,pad); if(!r.isOk()) return {r.error}; return SafeFileWriter::writeBytes(path,*r.value,{overwrite,true}); }
Result<std::vector<QImage>> IcoCodec::readIcoEntries(const QString& path) {
    QFile file(path); if(!file.open(QIODevice::ReadOnly)) return Result<std::vector<QImage>>::failure(ErrorCode::FileReadError,"No se pudo leer el icono",file.errorString());
    const QByteArray b=file.readAll(); auto bad=[] {return Result<std::vector<QImage>>::failure(ErrorCode::CorruptedInput,"Contenedor ICO inválido o entrada truncada");};
    if(b.size()<6 || qFromLittleEndian<quint16>(b.constData())!=0 || qFromLittleEndian<quint16>(b.constData()+2)!=1) return bad();
    const auto count=qFromLittleEndian<quint16>(b.constData()+4); if(!count || b.size()<6+16*count) return bad();
    std::vector<QImage> images;
    for(int i=0;i<count;++i) {
        const char* e=b.constData()+6+16*i; quint32 size=qFromLittleEndian<quint32>(e+8),offset=qFromLittleEndian<quint32>(e+12);
        if(offset<quint32(6+16*count) || quint64(offset)+size>quint64(b.size())) return bad();
        QImage image;
        if(b.mid(offset,8)==QByteArray::fromHex("89504e470d0a1a0a")) image=QImage::fromData(b.mid(offset,size),"PNG");
        else { // Let Qt decode traditional Windows DIB + AND-mask entries in a single-entry ICO.
            QByteArray single; u16(single,0); u16(single,1); u16(single,1); single.append(e,12); u32(single,22); single+=b.mid(offset,size); image=QImage::fromData(single,"ICO");
        }
        if(image.isNull()) return bad();
        images.push_back(image);
    }
    return Result<std::vector<QImage>>::success(images);
}
Result<FaviconBundleResult> IcoCodec::exportFaviconBundle(const QImage& src,const QString& dir,const FaviconExportOptions& o,bool overwrite) {
    using R=Result<FaviconBundleResult>;
    if(src.isNull() || o.paddingPercent<0 || o.paddingPercent>49) return R::failure(ErrorCode::InvalidParameter,"Imagen o margen inválidos");
    struct Pending { QString path; QByteArray bytes; }; std::vector<Pending> pending; FaviconBundleResult result;
    auto ico=encodeMultiResolutionIco(src,o.icoSizes,o.backgroundColor,o.fitMode,o.paddingPercent); if(!ico.isOk()) return R::failure(ico.error.code,ico.error.message,ico.error.detail);
    result.icoFilePath=QDir(dir).filePath("favicon.ico"); pending.push_back({result.icoFilePath,*ico.value}); QJsonArray icons;
    std::set<int> seen;
    for(int n:o.pngSizes) {
        if(n<1 || n>8192) return R::failure(ErrorCode::InvalidParameter,"Tamaño PNG de favicon fuera de 1..8192"); if(!seen.insert(n).second) continue;
        QString name=n==180?"apple-touch-icon-180x180.png":(n==192 || n==512?QString("android-chrome-%1x%1.png").arg(n):QString("favicon-%1x%1.png").arg(n));
        auto png=pngBytes(renderIcon(src,n,o.backgroundColor,o.fitMode,o.paddingPercent)); if(!png.isOk()) return R::failure(png.error.code,png.error.message);
        QString path=QDir(dir).filePath(name); pending.push_back({path,*png.value}); result.pngFilePaths<<path;
        icons.append(QJsonObject{{"src",name},{"sizes",QString("%1x%1").arg(n)},{"type","image/png"}});
    }
    if(o.generateWebManifest) { result.manifestFilePath=QDir(dir).filePath("site.webmanifest"); QJsonObject manifest{{"name",o.appName},{"short_name",o.appShortName},{"icons",icons},{"theme_color",o.themeColor.name()},{"background_color",o.backgroundColor.name()},{"display","standalone"}}; pending.push_back({result.manifestFilePath,QJsonDocument(manifest).toJson()}); }
    if(!overwrite) for(const auto& p:pending) if(QFile::exists(p.path)) return R::failure(ErrorCode::ConflictDetected,"Ya existe un archivo del paquete",p.path);
    for(const auto& p:pending) { auto status=SafeFileWriter::writeBytes(p.path,p.bytes,{overwrite,true}); if(!status.isOk()) return R::failure(status.error.code,status.error.message,status.error.detail); }
    return R::success(result);
}
}
