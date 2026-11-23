#include "crop_transform.hpp"
#include <QtGui/QPainter>
#include <QtGui/QTransform>
#include <cmath>
#include <algorithm>
#include <QtGui/QColorSpace>
#include <QtCore/QBuffer>
namespace photo26::core {
double CropTransformOp::aspectRatioValue(AspectRatioPreset p, double w, double h) {
    switch (p) {
    case AspectRatioPreset::Free:
        return 0;
    case AspectRatioPreset::Ratio1_1:
        return 1;
    case AspectRatioPreset::Ratio3_4:
        return 3. / 4;
    case AspectRatioPreset::Ratio4_3:
        return 4. / 3;
    case AspectRatioPreset::Ratio16_9:
        return 16. / 9;
    case AspectRatioPreset::Ratio9_16:
        return 9. / 16;
    case AspectRatioPreset::Ratio2_3:
        return 2. / 3;
    case AspectRatioPreset::Ratio3_2:
        return 3. / 2;
    case AspectRatioPreset::Ratio21_9:
        return 21. / 9;
    case AspectRatioPreset::Custom:
        return std::isfinite(w) && std::isfinite(h) && w > 0 && h > 0 && std::isfinite(w / h)
                   ? w / h
                   : 0;
    }
    return 0;
}
QRect CropTransformOp::computeCenteredAspectCrop(const QSize &s, AspectRatioPreset p, double w,
                                                 double h) {
    return clampCropRect(QRect(QPoint(), s), s, p, w, h);
}
QRect CropTransformOp::clampCropRect(const QRect &r, const QSize &s, AspectRatioPreset p, double w,
                                     double h) {
    if (s.isEmpty() || r.isEmpty())
        return {};
    QRect c = r.intersected(QRect(QPoint(), s));
    if (c.isEmpty())
        return {};
    double a = aspectRatioValue(p, w, h);
    if (a > 0) {
        int cw = c.width(), ch = c.height();
        if (double(cw) / ch > a)
            cw = std::max(1, int(std::floor(ch * a)));
        else
            ch = std::max(1, int(std::floor(cw / a)));
        c = QRect(c.x() + (c.width() - cw) / 2, c.y() + (c.height() - ch) / 2, cw, ch);
    }
    return c;
}
RuleOfThirdsLines CropTransformOp::computeRuleOfThirds(const QRectF &r) {
    double x = r.x(), y = r.y(), w = r.width(), h = r.height();
    return {{x + w / 3, y, x + w / 3, y + h},
            {x + 2 * w / 3, y, x + 2 * w / 3, y + h},
            {x, y + h / 3, x + w, y + h / 3},
            {x, y + 2 * h / 3, x + w, y + 2 * h / 3}};
}
Result<QImage> CropTransformOp::crop(const QImage &s, const CropOptions &o) {
    if (s.isNull() || (o.preset == AspectRatioPreset::Custom &&
                       aspectRatioValue(o.preset, o.customAspectWidth, o.customAspectHeight) <= 0))
        return Result<QImage>::failure(ErrorCode::InvalidParameter,
                                       "Invalid image or aspect ratio");
    QRect c =
        clampCropRect(o.cropRect, s.size(), o.preset, o.customAspectWidth, o.customAspectHeight);
    if (c.isEmpty())
        return Result<QImage>::failure(ErrorCode::InvalidParameter, "Empty crop");
    return Result<QImage>::success(s.copy(c));
}
Result<QImage> CropTransformOp::transform(const QImage &s, const TransformOptions &o) {
    if (s.isNull() || o.rotateQuadrants90 % 90 || !std::isfinite(o.freeAngleDegrees))
        return Result<QImage>::failure(ErrorCode::InvalidParameter,
                                       "Rotation must use multiples of 90 degrees");
    QTransform t;
    t.rotate((o.rotateQuadrants90 % 360) + std::remainder(o.freeAngleDegrees, 360.));
    t.scale(o.flipHorizontal ? -1 : 1, o.flipVertical ? -1 : 1);
    QRectF bounds = t.mapRect(QRectF(QPointF(), s.size()));
    if (bounds.width() > 32768 || bounds.height() > 32768 ||
        bounds.width() * bounds.height() > 134217728.)
        return Result<QImage>::failure(ErrorCode::ImageDimensionsTooLarge,
                                       "Transformed image too large");
    QImage out = s.transformed(t, Qt::SmoothTransformation);
    if (out.isNull())
        return Result<QImage>::failure(ErrorCode::InternalError, "Image allocation failed");
    if (o.backgroundColor.alpha() != 0) {
        QImage canvas(out.size(), QImage::Format_ARGB32_Premultiplied);
        canvas.fill(o.backgroundColor);
        QPainter p(&canvas);
        p.drawImage(0, 0, out);
        p.end();
        canvas.setColorSpace(s.colorSpace());
        out = canvas;
    }
    return Result<QImage>::success(out);
}
} // namespace photo26::core
