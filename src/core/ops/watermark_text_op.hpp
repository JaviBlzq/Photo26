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
} // namespace photo26::core
