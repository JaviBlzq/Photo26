#include "format_registry.hpp"
#include <QImageReader>
#include <QImageWriter>
#include <QFileInfo>
#include <QBuffer>
#include <QtEndian>
namespace photo26::core {
std::vector<FormatCapability> FormatRegistry::allFormats() {
    const auto rd = QImageReader::supportedImageFormats(), wr = QImageWriter::supportedImageFormats();
    std::vector<FormatCapability> out;
    auto add = [&](QString id, QString name, QStringList extensions) -> FormatCapability& {
        FormatCapability f; f.id = id; f.displayName = name; f.extensions = extensions;
        for (const auto& e : extensions) { f.canRead |= rd.contains(e.toLatin1()); f.canWrite |= wr.contains(e.toLatin1()); }
        out.push_back(f); return out.back();
    };
    auto& jpg=add("jpg","JPEG",{"jpg","jpeg"}); jpg.supportsQuality=true; jpg.supportsExif=jpg.supportsXmp=jpg.supportsIptc=jpg.supportsIcc=true;
    auto& png=add("png","PNG",{"png"}); png.supportsAlpha=png.supportsLossless=png.supportsExif=png.supportsXmp=png.supportsIcc=true;
    auto& webp=add("webp","WebP",{"webp"}); webp.supportsAlpha=webp.supportsQuality=webp.supportsLossless=webp.supportsAnimation=webp.supportsExif=webp.supportsXmp=webp.supportsIcc=true;
    add("bmp","BMP",{"bmp"}).supportsLossless=true;
    auto& tif=add("tiff","TIFF",{"tif","tiff"}); tif.supportsAlpha=tif.supportsLossless=tif.supportsMultiPage=tif.supportsExif=tif.supportsIcc=true;
    auto& gif=add("gif","GIF",{"gif"}); gif.canWrite=true; gif.supportsAlpha=gif.supportsAnimation=true; gif.notes="Escritura GIF89a propia, paleta de hasta 256 colores por fotograma.";
    auto& ico=add("ico","Icono Windows",{"ico"}); ico.canRead=ico.canWrite=true; ico.supportsAlpha=ico.supportsLossless=ico.supportsMultiPage=true;
    for (const auto& id : {"ppm","pgm","pbm","xbm","xpm"}) add(id,QString(id).toUpper(),{id}).supportsLossless=true;
    for (const auto& id : {"svg","svgz"}) { auto& f=add(id,QString(id).toUpper(),{id}); f.canRead=true; f.canWrite=false; f.isVector=true; f.supportsAlpha=true; f.notes="Photo26 rasteriza SVG con QSvgRenderer; no convierte mapas de bits a vectores ni incrusta un bitmap en un contenedor SVG."; }
    auto& pdf=add("pdf","PDF",{"pdf"}); pdf.canRead=false; pdf.canWrite=true; pdf.supportsMultiPage=true;
    for (const auto& id : {"heic","heif","avif"}) {
        auto& f=add(id,QString(id).toUpper(),{id});
        if (QString(id)!="avif") { f.canRead |= rd.contains("heic") || rd.contains("heif"); f.canWrite |= wr.contains("heic") || wr.contains("heif"); }
        f.supportsAlpha=true; f.supportsQuality=true;
        f.notes=(!f.canRead || !f.canWrite) ? "Detectado por cabecera ISOBMFF; la lectura/escritura requiere el códec HEIF/AVIF del sistema y un plugin Qt compatible disponible en ejecución." : "Códec HEIF/AVIF disponible mediante plugin Qt.";
    }
    // Include additional runtime plugins without promising capabilities we cannot verify.
    for (const auto& id : rd + wr) { bool found=false; for (const auto& f:out) if(f.extensions.contains(QString::fromLatin1(id))) found=true; if(!found) add(QString::fromLatin1(id),QString::fromLatin1(id).toUpper(),{QString::fromLatin1(id)}); }
    return out;
}
std::optional<FormatCapability> FormatRegistry::findFormat(const QString& s) {
    QString id=s.trimmed().toLower(); if(id.startsWith('.')) id.remove(0,1);
    for (const auto& f:allFormats()) if(f.id==id || f.extensions.contains(id)) return f;
    return {};
}
QString FormatRegistry::detectFormatFromHeader(const QByteArray& b, const QString& hint) {
    if(b.startsWith("\x89PNG\r\n\x1a\n")) return "png";
    if(b.startsWith(QByteArray::fromHex("ffd8ff"))) return "jpg";
    if(b.startsWith("GIF87a") || b.startsWith("GIF89a")) return "gif";
    if(b.startsWith("RIFF") && b.mid(8,4)=="WEBP") return "webp";
    if(b.startsWith(QByteArray::fromHex("00000100"))) return "ico";
    if(b.startsWith(QByteArray("II*\0",4)) || b.startsWith(QByteArray("MM\0*",4))) return "tiff";
    if(b.startsWith("BM")) return "bmp";
    if(b.startsWith("%PDF-")) return "pdf";
    if(b.size()>=12 && b.mid(4,4)=="ftyp") {
        const quint32 size=qFromBigEndian<quint32>(b.constData()); const qsizetype end=qMin<qsizetype>(b.size(),size);
        for(qsizetype p=8;p+4<=end;p+=4) { if(p==12) continue; auto brand=b.mid(p,4); if(brand=="avif" || brand=="avis") return "avif"; }
        for(qsizetype p=8;p+4<=end;p+=4) { if(p==12) continue; auto brand=b.mid(p,4); if(brand=="heic" || brand=="heix" || brand=="hevc" || brand=="hevx") return "heic"; if(brand=="mif1" || brand=="msf1") return "heif"; }
    }
    if(b.startsWith(QByteArray::fromHex("1f8b")) && QFileInfo(hint).suffix().compare("svgz",Qt::CaseInsensitive)==0) return "svgz";
    if(b.contains("<svg") || b.contains(":svg")) return "svg";
    QBuffer buffer; buffer.setData(b); buffer.open(QIODevice::ReadOnly); QImageReader reader(&buffer); auto id=QString::fromLatin1(reader.format());
    if(auto f=findFormat(id); !id.isEmpty() && f) return f->id;
    if(auto f=findFormat(QFileInfo(hint).suffix())) return f->id;
    return {};
}
QStringList FormatRegistry::supportedReadExtensions() { QStringList r; for(const auto& f:allFormats()) if(f.canRead) r.append(f.extensions); r.removeDuplicates(); return r; }
QStringList FormatRegistry::supportedWriteExtensions() { QStringList r; for(const auto& f:allFormats()) if(f.canWrite) r.append(f.extensions); r.removeDuplicates(); return r; }
QString FormatRegistry::fileDialogReadFilter() { QStringList patterns; for(const auto& e:supportedReadExtensions()) patterns << "*."+e; QStringList filters{"Imágenes ("+patterns.join(' ')+")"}; for(const auto& f:allFormats()) if(f.canRead) { QStringList p; for(const auto& e:f.extensions) p << "*."+e; filters << f.displayName+" ("+p.join(' ')+")"; } return filters.join(";;"); }
QString FormatRegistry::fileDialogWriteFilterFor(const QString& id) { auto f=findFormat(id); if(!f || !f->canWrite) return {}; QStringList p; for(const auto& e:f->extensions) p << "*."+e; return f->displayName+" ("+p.join(' ')+")"; }
}
