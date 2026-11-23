#pragma once
#include "../common/result.hpp"
#include "../common/cancellation.hpp"
namespace photo26::core {
enum class PdfPageSizePreset { A4, Letter, Legal, A3, FitToImage };
enum class PdfOrientation { Portrait, Landscape, AutoFromImage };
struct PdfExportOptions { PdfPageSizePreset pageSize{PdfPageSizePreset::A4}; PdfOrientation orientation{PdfOrientation::Portrait}; double marginMm{10.0}; bool proportionalFit{true}; bool centerOnPage{true}; int dpi{300}; QColor pageBackground{Qt::white}; QString title{"Photo26 Document"}; };
class PdfExporter {
public:
    static StatusResult exportSingleImageToPdf(const QImage& image,const QString& targetPdfPath,const PdfExportOptions& options = {},bool allowOverwrite = true);
    static StatusResult exportImagesToPdf(const std::vector<QImage>& images,const QString& targetPdfPath,const PdfExportOptions& options = {},bool allowOverwrite = true,const CancellationToken& cancelToken = {});
    static StatusResult exportImageFilesToPdf(const QStringList& imagePaths,const QString& targetPdfPath,const PdfExportOptions& options = {},bool allowOverwrite = true,const CancellationToken& cancelToken = {},const ProgressCallback& progressCb = {});
};
}
