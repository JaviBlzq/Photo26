#include "gif_codec.hpp"
#include "../common/safe_file_writer.hpp"
#include <QBuffer>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QImageWriter>
#include <QtEndian>
#include <array>
#include <unordered_map>
#include <cmath>
namespace photo26::core {
namespace {
void le16(QByteArray& b,quint16 n) { char p[2];qToLittleEndian(n,p);b.append(p,2); }
QByteArray quantize(const QImage& input,bool dither) {
    QImage image=input.convertToFormat(QImage::Format_ARGB32); QByteArray indices(image.width()*image.height(),Qt::Uninitialized);
    std::vector<std::array<double,3>> current(image.width()+2),next(image.width()+2);
    for(int y=0;y<image.height();++y) {
        const QRgb* row=reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for(int x=0;x<image.width();++x) {
            if(qAlpha(row[x])<128) { indices[y*image.width()+x]=0;continue; }
            const int channels[]{qRed(row[x]),qGreen(row[x]),qBlue(row[x])}; int q[3];
            for(int c=0;c<3;++c) { double v=qBound(0.,channels[c]+(dither?current[x+1][c]:0.),255.);q[c]=qBound(0,int(std::lround(v/51.)),5);if(dither) { double e=v-q[c]*51.;current[x+2][c]+=e*7/16;next[x][c]+=e*3/16;next[x+1][c]+=e*5/16;next[x+2][c]+=e/16; } }
            indices[y*image.width()+x]=char(1+q[0]*36+q[1]*6+q[2]);
        }
        current.swap(next);std::fill(next.begin(),next.end(),std::array<double,3>{});
    }
    return indices;
}
QByteArray lzw(const QByteArray& indices) {
    QByteArray packed; quint32 accumulator=0;int used=0,bits=9,next=258;std::unordered_map<quint32,int> dict;
    auto emitCode=[&](int code) { accumulator|=quint32(code)<<used;used+=bits;while(used>=8){packed.append(char(accumulator&255));accumulator>>=8;used-=8;} };
    emitCode(256);int prefix=quint8(indices[0]);
    for(qsizetype i=1;i<indices.size();++i) { int c=quint8(indices[i]);quint32 key=(quint32(prefix)<<8)|c;auto it=dict.find(key);if(it!=dict.end()) {prefix=it->second;continue;}
        emitCode(prefix);
        if(next<4096) {dict.emplace(key,next++);if(next>(1<<bits) && bits<12)++bits;}
        else {emitCode(256);dict.clear();bits=9;next=258;}
        prefix=c;
    }
    emitCode(prefix);
    // The decoder adds its last pending dictionary entry after the final data code.
    if(next==(1<<bits) && bits<12)++bits;
    emitCode(257);if(used)packed.append(char(accumulator&255));return packed;
}
}
Result<QByteArray> GifCodec::encodeAnimatedGif(const std::vector<GifFrame>& frames,const GifEncodeOptions& o) {
    using R=Result<QByteArray>;
    if(frames.empty() || frames.front().image.isNull() || o.targetWidth<0 || o.targetHeight<0 || o.loopCount<0 || o.loopCount>65535 || o.defaultDelayMs<1 || o.defaultDelayMs>655350) return R::failure(ErrorCode::InvalidParameter,"Fotogramas u opciones GIF inválidos");
    QSize size=frames.front().image.size();if(o.targetWidth && o.targetHeight)size={o.targetWidth,o.targetHeight};else if(o.targetWidth)size={o.targetWidth,qMax(1,qRound(double(size.height())*o.targetWidth/size.width()))};else if(o.targetHeight)size={qMax(1,qRound(double(size.width())*o.targetHeight/size.height())),o.targetHeight};
    if(size.width()<1 || size.height()<1 || size.width()>65535 || size.height()>65535 || quint64(size.width())*size.height()>134217728) return R::failure(ErrorCode::ImageDimensionsTooLarge,"Dimensiones GIF fuera de los límites");
    QByteArray out("GIF89a");le16(out,quint16(size.width()));le16(out,quint16(size.height()));out.append(char(0xf0));out.append(char(0));out.append(char(0));out.append(QByteArray(6,0));
    out.append(QByteArray::fromHex("21ff0b"));out.append("NETSCAPE2.0",11);out.append(char(3));out.append(char(1));le16(out,quint16(o.loopCount));out.append(char(0));
    QByteArray palette(256*3,0);for(int r=0;r<6;++r)for(int g=0;g<6;++g)for(int b=0;b<6;++b){int index=(1+r*36+g*6+b)*3;palette[index]=char(r*51);palette[index+1]=char(g*51);palette[index+2]=char(b*51);}
    for(const auto& frame:frames) {
        if(frame.image.isNull() || frame.delayMs>655350) return R::failure(ErrorCode::InvalidParameter,"Fotograma vacío o duración GIF excesiva");
        QImage image=frame.image.size()==size?frame.image:frame.image.scaled(size,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        auto pixels=quantize(image,o.dither);auto compressed=lzw(pixels);
        out.append(QByteArray::fromHex("21f90409"));le16(out,quint16(qBound(1,qRound((frame.delayMs>0?frame.delayMs:o.defaultDelayMs)/10.),65535)));out.append(char(0));out.append(char(0));
        out.append(char(0x2c));le16(out,0);le16(out,0);le16(out,quint16(size.width()));le16(out,quint16(size.height()));out.append(char(0x87));out+=palette;out.append(char(8));
        for(qsizetype p=0;p<compressed.size();p+=255){int n=int(qMin<qsizetype>(255,compressed.size()-p));out.append(char(n));out.append(compressed.constData()+p,n);}out.append(char(0));
    }
    out.append(char(0x3b));return R::success(out);
}
StatusResult GifCodec::saveAnimatedGif(const std::vector<GifFrame>& frames,const QString& path,const GifEncodeOptions& o,bool overwrite) { auto r=encodeAnimatedGif(frames,o);if(!r.isOk())return {r.error};return SafeFileWriter::writeBytes(path,*r.value,{overwrite,true}); }
Result<std::vector<GifFrame>> GifCodec::extractGifFrames(const QString& path) {
    using R=Result<std::vector<GifFrame>>;if(!QFileInfo::exists(path))return R::failure(ErrorCode::FileNotFound,"GIF inexistente",path);
    QImageReader reader(path,"gif");std::vector<GifFrame> frames;
    while(reader.canRead()) { QSize s=reader.size();if(s.isValid() && quint64(s.width())*s.height()>134217728) return R::failure(ErrorCode::ImageDimensionsTooLarge,"GIF demasiado grande");QImage image=reader.read();if(image.isNull())return R::failure(ErrorCode::CorruptedInput,"No se pudo leer un fotograma GIF",reader.errorString());frames.push_back({image,reader.nextImageDelay()}); }
    if(frames.empty())return R::failure(ErrorCode::CorruptedInput,"GIF vacío o inválido",reader.errorString());return R::success(frames);
}
Result<QStringList> GifCodec::exportGifFramesToFolder(const QString& path,const QString& dir,const QString& base,const QString& format,bool overwrite) {
    using R=Result<QStringList>;if(base.isEmpty() || base.contains('/') || base.contains('\\') || base=="..")return R::failure(ErrorCode::InvalidParameter,"Nombre base inválido");
    auto frames=extractGifFrames(path);if(!frames.isOk())return R::failure(frames.error.code,frames.error.message,frames.error.detail);QStringList paths;
    QByteArray fmt=format.toLower().toLatin1();if(!QImageWriter::supportedImageFormats().contains(fmt))return R::failure(ErrorCode::UnsupportedFormat,"No hay escritor Qt para el formato de fotogramas",format);
    for(size_t i=0;i<frames.value->size();++i){QString dest=QDir(dir).filePath(QString("%1_%2.%3").arg(base).arg(i+1,4,10,QChar('0')).arg(format));if(!overwrite && QFileInfo::exists(dest))return R::failure(ErrorCode::ConflictDetected,"Ya existe un fotograma",dest);paths<<dest;}
    for(size_t i=0;i<frames.value->size();++i){auto r=SafeFileWriter::writeWithCallback(paths[int(i)],[&](const QString& tmp){QImageWriter w(tmp,fmt);if(!w.write((*frames.value)[i].image))return StatusResult::failure(ErrorCode::FileWriteError,"No se pudo escribir el fotograma",w.errorString());return StatusResult::success();},{overwrite,true});if(!r.isOk())return R::failure(r.error.code,r.error.message,r.error.detail);}
    return R::success(paths);
}
}
