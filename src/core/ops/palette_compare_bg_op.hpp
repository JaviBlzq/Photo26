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
#ifndef PHOTO26_OP_PALETTE_COMPARE_BG_OP
#define PHOTO26_OP_PALETTE_COMPARE_BG_OP
struct PaletteColor {
    QColor color;
    QString hex;
    int r{0};
    int g{0};
    int b{0};
    double dominanceRatio{0.0};
};
enum class PaletteExportFormat { Json, CssVariables, GimpGpl, SwatchPng };
class PaletteExtractorOp {
  public:
    static Result<std::vector<PaletteColor>> extractDominantColors(const QImage &, int = 6);
    static Result<QByteArray> exportPalette(const std::vector<PaletteColor> &, PaletteExportFormat,
                                            const QString & = "Photo26 Palette");
    static Result<QImage> renderPaletteSwatch(const std::vector<PaletteColor> &, int = 720,
                                              int = 160);
};
enum class ComparisonMode { SideBySide, SplitSlider, OverlayBlend, VisualDifference };
struct ComparisonMetrics {
    QSize sizeA;
    QSize sizeB;
    std::uint64_t bytesA{0};
    std::uint64_t bytesB{0};
    double meanAbsoluteError{0.0};
    double rmse{0.0};
    double psnrDb{0.0};
    double diffPixelPercent{0.0};
    QImage renderedView;
};
class ImageCompareOp {
  public:
    static Result<ComparisonMetrics> compare(const QImage &, const QImage &,
                                             ComparisonMode = ComparisonMode::VisualDifference,
                                             double = 0.5, std::uint64_t = 0, std::uint64_t = 0);
};
enum class BgRemovalMethod { AutoBorderFloodAndGrabCut, ChromaKeyColor, LuminanceThreshold };
struct BgRemovalOptions {
    BgRemovalMethod method{BgRemovalMethod::AutoBorderFloodAndGrabCut};
    QColor keyColor{Qt::white};
    double tolerance{0.18};
    double featherRadius{1.5};
    bool replaceWithSolidColor{false};
    QColor replacementColor{Qt::white};
    QString optionalOnnxModelPath;
};
class BackgroundRemovalOp {
  public:
    static Result<QImage> removeBackground(const QImage &, const BgRemovalOptions & = {});
    // Standard build uses local border segmentation, no ONNX/model download and no GrabCut
    // dependency.
    static bool isOnnxRuntimeAvailable();
};
#endif
} // namespace photo26::core
