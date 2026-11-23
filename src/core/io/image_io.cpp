#include "image_io.hpp"
#include "format_registry.hpp"
#include "gif_codec.hpp"
#include "ico_codec.hpp"
#include "pdf_exporter.hpp"
#include "../common/safe_file_writer.hpp"
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QImageWriter>
#include <QSvgRenderer>
#include <QPainter>
#include <QBuffer>
#include <QDateTime>
#include <QColorSpace>
namespace photo26::core {
namespace {
bool allowed(QSize s,const ImageLimits& l){return s.width()>0&&s.height()>0&&s.width()<=l.maxWidth&&s.height()<=l.maxHeight&&quint64(s.width())*s.height()<=l.maxPixels;}
}
Result<LoadedImage> ImageIo::loadImage(const QString& path,const LoadOptions& o){using R=Result<LoadedImage>;QFileInfo fi(path);if(!fi.exists()||!fi.isFile())return R::failure(ErrorCode::FileNotFound,"Imagen inexistente",path);if(o.limits.maxWidth<1||o.limits.maxHeight<1||!o.limits.maxPixels||o.svgWidth<0||o.svgHeight<0)return R::failure(ErrorCode::InvalidParameter,"Límites de imagen inválidos");if(quint64(fi.size())>o.limits.maxFileBytes)return R::failure(ErrorCode::ImageDimensionsTooLarge,"El archivo supera el límite de tamaño");QFile in(path);if(!in.open(QIODevice::ReadOnly))return R::failure(ErrorCode::FileReadError,"No se pudo abrir la imagen",in.errorString());QString format=FormatRegistry::detectFormatFromHeader(in.peek(65536),path);auto capability=FormatRegistry::findFormat(format);if(!capability||!capability->canRead)return R::failure(ErrorCode::UnsupportedFormat,"Formato no disponible para lectura; HEIC/HEIF/AVIF requieren un códec del sistema y plugin Qt compatible",format);
    LoadedImage out;out.sourcePath=fi.absoluteFilePath();out.detectedFormat=format;
    if(format=="svg"||format=="svgz"){QSvgRenderer renderer(path);if(!renderer.isValid())return R::failure(ErrorCode::CorruptedInput,"SVG/SVGZ no válido");QSize size=renderer.defaultSize();if(o.svgWidth&&o.svgHeight)size={o.svgWidth,o.svgHeight};else if(o.svgWidth&&size.width()>0)size={o.svgWidth,qMax(1,qRound(double(size.height())*o.svgWidth/size.width()))};else if(o.svgHeight&&size.height()>0)size={qMax(1,qRound(double(size.width())*o.svgHeight/size.height())),o.svgHeight};if(!allowed(size,o.limits))return R::failure(ErrorCode::ImageDimensionsTooLarge,"Rasterización SVG fuera de los límites");out.image=QImage(size,QImage::Format_ARGB32_Premultiplied);if(out.image.isNull())return R::failure(ErrorCode::InternalError,"No hay memoria para rasterizar SVG");out.image.fill(Qt::transparent);QPainter painter(&out.image);renderer.render(&painter,QRectF(QPointF(0,0),size));painter.end();}
    else {const QByteArray fmtHint = QImageReader::supportedImageFormats().contains(format.toLatin1()) ? format.toLatin1() : QByteArray(); QImageReader reader(&in, fmtHint);reader.setAutoTransform(false);QSize size=reader.size();if(size.isValid()&&!allowed(size,o.limits))return R::failure(ErrorCode::ImageDimensionsTooLarge,"Dimensiones de imagen fuera de los límites");out.image=reader.read();if(out.image.isNull())return R::failure(ErrorCode::CorruptedInput,"No se pudo decodificar la imagen",reader.errorString());if(!allowed(out.image.size(),o.limits))return R::failure(ErrorCode::ImageDimensionsTooLarge,"Dimensiones decodificadas fuera de los límites");}
    out.fileProps={fi.fileName(),fi.absoluteFilePath(),quint64(fi.size()),fi.birthTime().toString(Qt::ISODateWithMs),fi.lastModified().toString(Qt::ISODateWithMs),out.image.width(),out.image.height(),out.image.depth(),out.image.hasAlphaChannel(),format};
    if(o.readMetadata||o.autoOrient){auto inspection=MetadataEngine::inspectFile(path);if(inspection.isOk()){if(o.readMetadata){out.metadata=inspection.value->embedded;out.fileProps=inspection.value->fileProps;}else out.metadata.orientation=inspection.value->embedded.orientation;}else if(o.readMetadata)return R::failure(inspection.error.code,inspection.error.message,inspection.error.detail);}
    if(o.autoOrient&&out.metadata.orientation>1){out.image=MetadataEngine::applyExifOrientation(out.image,out.metadata.orientation);out.metadata.orientation=1;}
    if(!o.readMetadata)out.metadata=ImageMetadata{};return R::success(out);
}
StatusResult ImageIo::saveImage(const QImage& image,const QString& path,const SaveOptions& o,const ImageMetadata* metadata,bool overwrite,QStringList* warnings){if(image.isNull()||path.isEmpty()||o.quality< -1||o.quality>100)return StatusResult::failure(ErrorCode::InvalidParameter,"Imagen, calidad o ruta inválida");QString format=o.format.isEmpty()?QFileInfo(path).suffix():o.format;auto capability=FormatRegistry::findFormat(format);if(!capability)return StatusResult::failure(ErrorCode::UnsupportedFormat,"Formato de destino desconocido",format);format=capability->id;if(format=="svg"||format=="svgz")return StatusResult::failure(ErrorCode::UnsupportedFormat,"Photo26 no convierte mapas de bits a vectores ni envuelve imágenes raster dentro de SVG");if(!capability->canWrite)return StatusResult::failure(ErrorCode::UnsupportedFormat,"Escritor no disponible; HEIC/HEIF/AVIF necesitan plugin Qt y códec del sistema",capability->notes);
    QImage output=image;if(format=="jpg"||format=="bmp"||format=="ppm"||!o.preserveTransparency){QColor background=o.backgroundColor;background.setAlpha(255);QImage flat(image.size(),QImage::Format_RGB32);if(flat.isNull())return StatusResult::failure(ErrorCode::InternalError,"No hay memoria para aplanar transparencia");flat.fill(background);flat.setColorSpace(image.colorSpace());QPainter p(&flat);p.drawImage(0,0,image);p.end();output=flat;}
    if(!o.preserveIccProfile)output.setColorSpace(QColorSpace());
    if(!o.preserveMetadata)for(const auto& key:output.textKeys())output.setText(key,{});
    // Metadata is written into the temporary image before the destination is committed.
    return SafeFileWriter::writeWithCallback(path,[&](const QString& tmp){StatusResult result;
        if(format=="pdf")result=PdfExporter::exportSingleImageToPdf(output,tmp,{},true);
        else if(format=="ico")result=IcoCodec::saveMultiResolutionIco(output,tmp,{16,32,48},Qt::transparent,IconFitMode::ContainProportional,0,true);
        else if(format=="gif")result=GifCodec::saveAnimatedGif({{output,100}},tmp,{},true);
        else {QImageWriter writer(tmp,format.toLatin1());writer.setQuality(o.lossless&&format=="webp"?100:o.quality);if(o.lossless&&format=="webp")writer.setCompression(100);if(!writer.write(output))result=StatusResult::failure(ErrorCode::FileWriteError,"No se pudo codificar la imagen",writer.errorString());else result=StatusResult::success();}
        if(!result.isOk())return result;
        if(o.preserveMetadata&&metadata){if(format=="pdf"){if(warnings)*warnings<<"PDF solo conserva el título de documento; no admite los metadatos fotográficos completos.";}else {ImageMetadata m=*metadata;if(!o.preserveIccProfile){m.iccProfileBytes.clear();m.hasIcc=false;m.colorProfileName.clear();}result=MetadataEngine::writeMetadataToFile(tmp,m,warnings);if(!result.isOk())return result;}}
        return StatusResult::success();
    },{overwrite,true});
}
}
