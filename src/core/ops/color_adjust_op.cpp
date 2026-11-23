#include "color_adjust_op.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <QtGui/QColorSpace>
namespace photo26::core {
Result<QImage> ColorAdjustOp::apply(const QImage &source, const ColorAdjustOptions &o) {
    auto valid = [](double v, double lo, double hi) {
        return std::isfinite(v) && v >= lo && v <= hi;
    };
    if (source.isNull() || !valid(o.brightness, -100, 100) || !valid(o.contrast, -100, 100) ||
        !valid(o.saturation, -100, 100) || !valid(o.gamma, .1, 5) || !valid(o.exposureEv, -5, 5) ||
        !valid(o.sepiaIntensity, 0, 1) || o.bwThreshold < 0 || o.bwThreshold > 255)
        return Result<QImage>::failure(ErrorCode::InvalidParameter, "Invalid color adjustment");
    QImage working = source;
    if (working.colorSpace().isValid() && working.colorSpace() != QColorSpace(QColorSpace::SRgb))
        working = working.convertedToColorSpace(QColorSpace::SRgb);
    QImage out = working.convertToFormat(QImage::Format_RGBA64);
    if (out.isNull())
        return Result<QImage>::failure(ErrorCode::InternalError, "Allocation failed");
    std::vector<double> err(source.width() + 2), next(err.size());
    double exposure = std::exp2(o.exposureEv), contrast = std::pow((100. + o.contrast) / 100., 2.);
    auto clamp = [](double v) { return std::clamp(v, 0., 1.); };
    // EV is applied to normalized linear-light sRGB, then encoded back before tone controls.
    auto linear = [](double v) {
        return v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4);
    };
    auto encoded = [](double v) {
        return v <= .0031308 ? 12.92 * v : 1.055 * std::pow(v, 1 / 2.4) - .055;
    };
    for (int y = 0; y < out.height(); ++y) {
        auto row = reinterpret_cast<QRgba64 *>(out.scanLine(y));
        std::fill(next.begin(), next.end(), 0.);
        for (int x = 0; x < out.width(); ++x) {
            double r = row[x].red() / 65535., g = row[x].green() / 65535.,
                   b = row[x].blue() / 65535.;
            auto tone = [&](double v) {
                if (o.exposureEv != 0)
                    v = encoded(clamp(linear(v) * exposure));
                v = clamp((v - .5) * contrast + .5 + o.brightness / 100.);
                return std::pow(v, 1 / o.gamma);
            };
            r = tone(r);
            g = tone(g);
            b = tone(b);
            double lum = .2126 * r + .7152 * g + .0722 * b, f = 1 + o.saturation / 100.;
            r = clamp(lum + (r - lum) * f);
            g = clamp(lum + (g - lum) * f);
            b = clamp(lum + (b - lum) * f);
            if (o.grayscale)
                r = g = b = .2126 * r + .7152 * g + .0722 * b;
            if (o.sepia) {
                double nr = clamp(.393 * r + .769 * g + .189 * b),
                       ng = clamp(.349 * r + .686 * g + .168 * b),
                       nb = clamp(.272 * r + .534 * g + .131 * b);
                r += (nr - r) * o.sepiaIntensity;
                g += (ng - g) * o.sepiaIntensity;
                b += (nb - b) * o.sepiaIntensity;
            }
            if (o.invert) {
                r = 1 - r;
                g = 1 - g;
                b = 1 - b;
            }
            if (o.blackAndWhite) {
                double v = .2126 * r + .7152 * g + .0722 * b + (o.bwDither ? err[x + 1] : 0),
                       bw = v >= o.bwThreshold / 255. ? 1 : 0;
                if (o.bwDither) {
                    double e = (v - bw) * (row[x].alpha() / 65535.);
                    err[x + 2] += e * 7 / 16;
                    next[x] += e * 3 / 16;
                    next[x + 1] += e * 5 / 16;
                    next[x + 2] += e / 16;
                }
                r = g = b = bw;
            }
            row[x] = QRgba64::fromRgba64(ushort(std::lround(clamp(r) * 65535)),
                                         ushort(std::lround(clamp(g) * 65535)),
                                         ushort(std::lround(clamp(b) * 65535)), row[x].alpha());
        }
        err.swap(next);
    }
    return Result<QImage>::success(out);
}
} // namespace photo26::core
