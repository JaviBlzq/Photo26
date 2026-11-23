#pragma once
#include "../common/types.hpp"
#include "../common/result.hpp"
#include "../common/cancellation.hpp"
#include "../common/safe_file_writer.hpp"
#include <QtCore/QRect>
#include <QtCore/QLineF>
#include <QtCore/QJsonObject>
#include <functional>
namespace photo26::core {
#ifndef PHOTO26_OP_RESIZE_OP
#define PHOTO26_OP_RESIZE_OP
enum class ResizeMode { ByPixels, ByPercentage, FitMaxDimensions };
enum class InterpolationMethod { Nearest, Bilinear, Bicubic, Lanczos3 };
struct ResizeOptions {
    ResizeMode mode{ResizeMode::ByPixels};
    int targetWidth{0};
    int targetHeight{0};
    double percentage{100.0};
    int maxWidth{1920};
    int maxHeight{1080};
    bool keepAspectRatio{true};
    bool preventUpscaling{false};
    InterpolationMethod interpolation{InterpolationMethod::Lanczos3};
};
class ResizeOp {
  public:
    static Result<QSize> computeTargetSize(const QSize &, const ResizeOptions &);
    static Result<QImage> resize(const QImage &, const ResizeOptions &);
};
#endif
#ifndef PHOTO26_OP_COLOR_ADJUST_OP
#define PHOTO26_OP_COLOR_ADJUST_OP
struct ColorAdjustOptions {
    bool grayscale{false};
    bool blackAndWhite{false};
    int bwThreshold{128};
    bool bwDither{false};
    bool sepia{false};
    double sepiaIntensity{1.0};
    bool invert{false};
    double brightness{0.0};
    double contrast{0.0};
    double saturation{0.0};
    double gamma{1.0};
    double exposureEv{0.0};
};
class ColorAdjustOp {
  public:
    static Result<QImage> apply(const QImage &, const ColorAdjustOptions &);
};
#endif
#ifndef PHOTO26_OP_WATERMARK_TEXT_OP
#define PHOTO26_OP_WATERMARK_TEXT_OP
enum class AnchorPosition {
    TopLeft,
    TopCenter,
    TopRight,
    MiddleLeft,
    Center,
    MiddleRight,
    BottomLeft,
    BottomCenter,
    BottomRight,
    Tiled
};
struct WatermarkOptions {
    bool useImageWatermark{false};
    QString text{"© Photo26"};
    QString fontFamily{"Segoe UI"};
    int fontPointSize{24};
    bool bold{true};
    QColor textColor{255, 255, 255, 220};
    bool textOutline{true};
    QColor outlineColor{0, 0, 0, 180};
    QImage watermarkImage;
    double opacity{0.65};
    double scalePercent{20.0};
    AnchorPosition position{AnchorPosition::BottomRight};
    int marginX{20};
    int marginY{20};
    double rotationDegrees{0.0};
};
struct TextAndBorderOptions {
    QString text;
    QString fontFamily{"Segoe UI"};
    int fontPointSize{28};
    bool bold{false};
    bool italic{false};
    QColor textColor{Qt::white};
    QColor textBoxBackground{Qt::transparent};
    AnchorPosition textPosition{AnchorPosition::BottomCenter};
    int textMargin{24};
    int paddingPx{0};
    int borderWidthPx{0};
    QColor borderColor{Qt::black};
    int cornerRadiusPx{0};
    QColor canvasBackground{Qt::transparent};
};
class WatermarkTextOp {
  public:
    static Result<QImage> applyWatermark(const QImage &, const WatermarkOptions &);
    static Result<QImage> applyTextAndBorders(const QImage &, const TextAndBorderOptions &);
};
#endif
struct MassRenameOptions {
    QString pattern{"{name}"};
    QString prefix;
    QString suffix;
    QString findText;
    QString replaceText;
    int startNumber{1};
    int numberStep{1};
    int zeroPadding{3};
    QString forceExtension;
};
struct RenamePreviewItem {
    QString originalPath;
    QString originalName;
    QString proposedName;
    QString proposedPath;
    bool hasConflict{false};
    QString conflictReason;
};
class MassRenameOp {
  public:
    static std::vector<RenamePreviewItem>
    previewRename(const QStringList &, const MassRenameOptions &, const QString & = {});
    static StatusResult executeRenameOrCopy(const std::vector<RenamePreviewItem> &, bool = true,
                                            bool = false);
};
struct BatchPreset {
    QString name{"Default"};
    QString outputFormat{"webp"};
    int quality{85};
    bool lossless{false};
    bool stripMetadata{false};
    bool enableResize{false};
    ResizeOptions resizeOptions{};
    bool enableColorAdjust{false};
    ColorAdjustOptions colorOptions{};
    bool enableWatermark{false};
    WatermarkOptions watermarkOptions{};
    MassRenameOptions renameOptions{};
    QColor backgroundColor{Qt::white};
};
struct BatchItemResult {
    QString sourcePath;
    QString outputPath;
    bool success{false};
    bool skipped{false};
    QString errorMessage;
    std::uint64_t inputBytes{0};
    std::uint64_t outputBytes{0};
};
struct BatchSummary {
    int totalItems{0};
    int succeeded{0};
    int failed{0};
    int cancelled{0};
    std::uint64_t totalInputBytes{0};
    std::uint64_t totalOutputBytes{0};
    std::vector<BatchItemResult> itemResults;
};
using CustomBatchImageSaver = std::function<StatusResult(const QImage &, const QString &,
                                                         const QString &, const BatchPreset &)>;
class BatchProcessor {
  public:
    static BatchSummary runBatch(const QStringList &, const QString &, const BatchPreset &,
                                 bool = false, const CancellationToken & = {},
                                 const ProgressCallback & = {}, int = 0,
                                 const CustomBatchImageSaver & = {});
    static QJsonObject presetToJson(const BatchPreset &);
    static Result<BatchPreset> presetFromJson(const QJsonObject &);
};
QJsonObject presetToJson(const BatchPreset &);
Result<BatchPreset> presetFromJson(const QJsonObject &);
} // namespace photo26::core
