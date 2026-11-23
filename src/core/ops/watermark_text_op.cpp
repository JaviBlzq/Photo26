#include "watermark_text_op.hpp"
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QFontMetricsF>
#include <QtGui/QGuiApplication>
#include <QtGui/QColorSpace>
#include <algorithm>
#include <cmath>
namespace photo26::core {
namespace {
QPointF anchor(QSize canvas, QSizeF size, AnchorPosition pos, int mx, int my) {
    int n = int(pos);
    return {n % 3 == 0   ? double(mx)
            : n % 3 == 1 ? (canvas.width() - size.width()) / 2
                         : canvas.width() - size.width() - mx,
            n / 3 == 0   ? double(my)
            : n / 3 == 1 ? (canvas.height() - size.height()) / 2
                         : canvas.height() - size.height() - my};
}
QImage textImage(const QString &text, const QFont &font, const QColor &color, bool outline,
                 const QColor &oc, const QColor &bg) {
    QFontMetricsF fm(font);
    QRectF bounds = fm.boundingRect(QRectF(0, 0, 30000, 30000), Qt::TextExpandTabs, text);
    int w = int(std::ceil(bounds.width())) + 8, h = int(std::ceil(bounds.height())) + 8;
    if (w <= 0 || h <= 0 || w > 32768 || h > 32768 || 1LL * w * h > 134217728)
        return {};
    QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
    img.fill(bg);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    QPainterPath path;
    const auto lines = text.split('\n');
    double y = 4 + fm.ascent();
    for (auto &line : lines) {
        path.addText(4, y, font, line);
        y += fm.lineSpacing();
    }
    if (outline)
        p.strokePath(path, QPen(oc, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.fillPath(path, color);
    return img;
}
} // namespace
Result<QImage> WatermarkTextOp::applyWatermark(const QImage &source, const WatermarkOptions &o) {
    if (source.isNull() || !std::isfinite(o.opacity) || o.opacity < 0 || o.opacity > 1 ||
        !std::isfinite(o.scalePercent) || o.scalePercent <= 0 || o.scalePercent > 1000 ||
        !std::isfinite(o.rotationDegrees) || o.marginX < 0 || o.marginY < 0 || o.fontPointSize <= 0)
        return Result<QImage>::failure(ErrorCode::InvalidParameter, "Invalid watermark parameters");
    QImage mark;
    if (o.useImageWatermark) {
        if (o.watermarkImage.isNull())
            return Result<QImage>::failure(ErrorCode::InvalidParameter, "Missing watermark image");
        int w = std::max(1, int(source.width() * o.scalePercent / 100.));
        if (w > 32768)
            return Result<QImage>::failure(ErrorCode::ImageDimensionsTooLarge,
                                           "Watermark too large");
        mark = o.watermarkImage.scaledToWidth(w, Qt::SmoothTransformation);
    } else {
        if (!qobject_cast<QGuiApplication *>(QCoreApplication::instance()))
            return Result<QImage>::failure(ErrorCode::InvalidParameter,
                                           "Text rendering requires QGuiApplication");
        QFont f(o.fontFamily, o.fontPointSize);
        f.setBold(o.bold);
        mark = textImage(o.text, f, o.textColor, o.textOutline, o.outlineColor, Qt::transparent);
    }
    if (mark.isNull())
        return Result<QImage>::failure(ErrorCode::InvalidParameter, "Empty watermark");
    QTransform t;
    t.rotate(std::remainder(o.rotationDegrees, 360.));
    mark = mark.transformed(t, Qt::SmoothTransformation);
    QImage out = source.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QPainter p(&out);
    p.setOpacity(o.opacity);
    if (o.position == AnchorPosition::Tiled) {
        int dx = mark.width() + std::max(1, o.marginX), dy = mark.height() + std::max(1, o.marginY);
        for (int y = o.marginY; y < out.height(); y += dy)
            for (int x = o.marginX; x < out.width(); x += dx)
                p.drawImage(x, y, mark);
    } else
        p.drawImage(anchor(out.size(), mark.size(), o.position, o.marginX, o.marginY), mark);
    p.end();
    return Result<QImage>::success(out);
}
Result<QImage> WatermarkTextOp::applyTextAndBorders(const QImage &source,
                                                    const TextAndBorderOptions &o) {
    long long extra = 2LL * (static_cast<long long>(o.paddingPx) + o.borderWidthPx),
              w = source.width() + extra, h = source.height() + extra;
    if (source.isNull() || o.paddingPx < 0 || o.borderWidthPx < 0 || o.cornerRadiusPx < 0 ||
        o.textMargin < 0 || o.fontPointSize <= 0)
        return Result<QImage>::failure(ErrorCode::InvalidParameter, "Invalid text/border options");
    if (w > 32768 || h > 32768 || w * h > 134217728)
        return Result<QImage>::failure(ErrorCode::ImageDimensionsTooLarge, "Canvas too large");
    QImage out(int(w), int(h), QImage::Format_ARGB32_Premultiplied);
    if (out.isNull())
        return Result<QImage>::failure(ErrorCode::InternalError, "Allocation failed");
    out.fill(Qt::transparent);
    QPainter p(&out);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath shape;
    shape.addRoundedRect(QRectF(0, 0, w, h), o.cornerRadiusPx, o.cornerRadiusPx);
    p.setClipPath(shape);
    p.fillRect(out.rect(), o.canvasBackground);
    int inset = o.paddingPx + o.borderWidthPx;
    p.drawImage(inset, inset, source);
    if (o.borderWidthPx) {
        p.setPen(QPen(o.borderColor, o.borderWidthPx));
        p.setBrush(Qt::NoBrush);
        double d = o.borderWidthPx / 2.;
        p.drawRoundedRect(QRectF(d, d, w - o.borderWidthPx, h - o.borderWidthPx),
                          std::max(0., o.cornerRadiusPx - d), std::max(0., o.cornerRadiusPx - d));
    }
    if (!o.text.isEmpty()) {
        if (!qobject_cast<QGuiApplication *>(QCoreApplication::instance()))
            return Result<QImage>::failure(ErrorCode::InvalidParameter,
                                           "Text rendering requires QGuiApplication");
        QFont f(o.fontFamily, o.fontPointSize);
        f.setBold(o.bold);
        f.setItalic(o.italic);
        QImage mark =
            textImage(o.text, f, o.textColor, false, Qt::transparent, o.textBoxBackground);
        if (mark.isNull())
            return Result<QImage>::failure(ErrorCode::InvalidParameter, "Text image too large");
        if (o.textPosition == AnchorPosition::Tiled) {
            for (int y = o.textMargin; y < out.height(); y += mark.height() + 1)
                for (int x = o.textMargin; x < out.width(); x += mark.width() + 1)
                    p.drawImage(x, y, mark);
        } else
            p.drawImage(anchor(out.size(), mark.size(), o.textPosition, o.textMargin, o.textMargin),
                        mark);
    }
    p.end();
    out.setColorSpace(source.colorSpace());
    return Result<QImage>::success(out);
}
} // namespace photo26::core
