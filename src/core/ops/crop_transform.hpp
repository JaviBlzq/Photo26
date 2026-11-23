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
#ifndef PHOTO26_OP_CROP_TRANSFORM
#define PHOTO26_OP_CROP_TRANSFORM
enum class AspectRatioPreset {
    Free,
    Ratio1_1,
    Ratio3_4,
    Ratio4_3,
    Ratio16_9,
    Ratio9_16,
    Ratio2_3,
    Ratio3_2,
    Ratio21_9,
    Custom
};
struct CropOptions {
    QRect cropRect;
    AspectRatioPreset preset{AspectRatioPreset::Free};
    double customAspectWidth{1.0};
    double customAspectHeight{1.0};
};
struct TransformOptions {
    int rotateQuadrants90{0};
    double freeAngleDegrees{0.0};
    bool flipHorizontal{false};
    bool flipVertical{false};
    QColor backgroundColor{Qt::transparent};
};
struct RuleOfThirdsLines {
    QLineF vertical1, vertical2, horizontal1, horizontal2;
};
class CropTransformOp {
  public:
    static double aspectRatioValue(AspectRatioPreset, double = 1.0, double = 1.0);
    static QRect computeCenteredAspectCrop(const QSize &, AspectRatioPreset, double = 1.0,
                                           double = 1.0);
    static QRect clampCropRect(const QRect &, const QSize &,
                               AspectRatioPreset = AspectRatioPreset::Free, double = 1.0,
                               double = 1.0);
    static RuleOfThirdsLines computeRuleOfThirds(const QRectF &);
    static Result<QImage> crop(const QImage &, const CropOptions &);
    static Result<QImage> transform(const QImage &, const TransformOptions &);
};
#endif
} // namespace photo26::core
