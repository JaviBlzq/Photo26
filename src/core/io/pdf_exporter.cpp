#include "pdf_exporter.hpp"
#include "image_io.hpp"
#include "../common/safe_file_writer.hpp"
#include <QFile>
#include <QPdfWriter>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <cmath>
namespace photo26::core {
namespace {
StatusResult pdf(const QString& path,int count,const std::function<Result<QImage>(int)>& load,const PdfExportOptions& o,bool overwrite,const CancellationToken& cancel,const ProgressCallback& progress,const QStringList& names) {
    if(count<1 || o.dpi<1 || o.dpi>9600 || !std::isfinite(o.marginMm) || o.marginMm<0) return StatusResult::failure(ErrorCode::InvalidParameter,"Páginas, resolución o márgenes PDF inválidos");
    return SafeFileWriter::writeWithCallback(path,[&](const QString& tmp) {
        QFile device(tmp);if(!device.open(QIODevice::WriteOnly|QIODevice::Truncate))return StatusResult::failure(ErrorCode::FileWriteError,"No se pudo abrir PDF",device.errorString());
        {
            QPdfWriter writer(&device);writer.setResolution(o.dpi);writer.setTitle(o.title);writer.setCreator("Photo26");QPainter painter;
            for(int i=0;i<count;++i) {
                if(cancel.isCancelled())return StatusResult::failure(ErrorCode::Cancelled,"Exportación PDF cancelada");
                auto r=load(i);if(!r.isOk())return StatusResult{r.error};const QImage& image=*r.value;if(image.isNull())return StatusResult::failure(ErrorCode::InvalidParameter,"Página con imagen vacía");
                QPageSize size;switch(o.pageSize){case PdfPageSizePreset::Letter:size=QPageSize(QPageSize::Letter);break;case PdfPageSizePreset::Legal:size=QPageSize(QPageSize::Legal);break;case PdfPageSizePreset::A3:size=QPageSize(QPageSize::A3);break;case PdfPageSizePreset::FitToImage:size=QPageSize(QSizeF(image.width()*25.4/o.dpi+2*o.marginMm,image.height()*25.4/o.dpi+2*o.marginMm),QPageSize::Millimeter,"Imagen",QPageSize::ExactMatch);break;default:size=QPageSize(QPageSize::A4);}
                bool landscape=o.orientation==PdfOrientation::Landscape || (o.orientation==PdfOrientation::AutoFromImage && image.width()>image.height());
                // Custom page dimensions already express the image's aspect ratio.
                if(o.pageSize==PdfPageSizePreset::FitToImage) landscape=false;
                QPageLayout layout(size,landscape?QPageLayout::Landscape:QPageLayout::Portrait,QMarginsF(o.marginMm,o.marginMm,o.marginMm,o.marginMm),QPageLayout::Millimeter);
                const QRectF full=layout.fullRect(QPageLayout::Millimeter);
                if(!layout.isValid() || 2*o.marginMm>=full.width() || 2*o.marginMm>=full.height() || !writer.setPageLayout(layout))return StatusResult::failure(ErrorCode::InvalidParameter,"Los márgenes no dejan área imprimible");
                if(i==0){if(!painter.begin(&writer))return StatusResult::failure(ErrorCode::FileWriteError,"No se pudo iniciar PDF");}
                else if(!writer.newPage())return StatusResult::failure(ErrorCode::FileWriteError,"No se pudo añadir página PDF");
                QRectF page=writer.pageLayout().fullRectPixels(o.dpi),paint=writer.pageLayout().paintRectPixels(o.dpi);
                // QPdfWriter's painter origin is the printable area.
                page.translate(-paint.topLeft());paint.moveTopLeft(QPointF(0,0));painter.fillRect(page,o.pageBackground);
                QSizeF target=image.size();if(o.proportionalFit)target.scale(paint.size(),Qt::KeepAspectRatio);else target=paint.size();
                QRectF rect(paint.topLeft(),target);if(o.centerOnPage)rect.moveCenter(paint.center());painter.drawImage(rect,image);
                if(progress)progress(i+1,count,names.isEmpty()?QString::number(i+1):names[i]);
            }
            if(cancel.isCancelled())return StatusResult::failure(ErrorCode::Cancelled,"Exportación PDF cancelada");
            if(!painter.end())return StatusResult::failure(ErrorCode::FileWriteError,"No se pudo terminar PDF");
        }
        if(!device.flush() || device.error()!=QFileDevice::NoError)return StatusResult::failure(ErrorCode::FileWriteError,"Error escribiendo PDF",device.errorString());return StatusResult::success();
    },{overwrite,true});
}
}
StatusResult PdfExporter::exportSingleImageToPdf(const QImage& image,const QString& path,const PdfExportOptions& o,bool overwrite){return exportImagesToPdf({image},path,o,overwrite);}
StatusResult PdfExporter::exportImagesToPdf(const std::vector<QImage>& images,const QString& path,const PdfExportOptions& o,bool overwrite,const CancellationToken& cancel){if(images.size()>size_t(INT_MAX))return StatusResult::failure(ErrorCode::InvalidParameter,"Demasiadas páginas");return pdf(path,int(images.size()),[&](int i){return Result<QImage>::success(images[size_t(i)]);},o,overwrite,cancel,{},{});}
StatusResult PdfExporter::exportImageFilesToPdf(const QStringList& paths,const QString& path,const PdfExportOptions& o,bool overwrite,const CancellationToken& cancel,const ProgressCallback& progress){if(paths.size()>INT_MAX)return StatusResult::failure(ErrorCode::InvalidParameter,"Demasiadas páginas");return pdf(path,int(paths.size()),[&](int i){auto loaded=ImageIo::loadImage(paths[i]);if(!loaded.isOk())return Result<QImage>::failure(loaded.error.code,loaded.error.message,loaded.error.detail);return Result<QImage>::success(loaded.value->image);},o,overwrite,cancel,progress,paths);}
}
