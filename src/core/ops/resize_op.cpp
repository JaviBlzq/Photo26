#include "resize_op.hpp"
#include <QtGui/QColorSpace>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>
namespace photo26::core {
Result<QSize> ResizeOp::computeTargetSize(const QSize &s, const ResizeOptions &o) {
    if (s.isEmpty())
        return Result<QSize>::failure(ErrorCode::InvalidParameter, "Empty source size");
    double w = o.targetWidth, h = o.targetHeight;
    if (o.mode == ResizeMode::ByPercentage) {
        if (!std::isfinite(o.percentage) || o.percentage <= 0)
            return Result<QSize>::failure(ErrorCode::InvalidParameter, "Invalid percentage");
        w = s.width() * o.percentage / 100.;
        h = s.height() * o.percentage / 100.;
    } else if (o.mode == ResizeMode::FitMaxDimensions) {
        if (o.maxWidth <= 0 || o.maxHeight <= 0)
            return Result<QSize>::failure(ErrorCode::InvalidParameter, "Invalid maximum size");
        double f = std::min(double(o.maxWidth) / s.width(), double(o.maxHeight) / s.height());
        w = s.width() * f;
        h = s.height() * f;
    } else {
        if (w <= 0 && h <= 0)
            return Result<QSize>::failure(ErrorCode::InvalidParameter, "Target size required");
        if (o.keepAspectRatio) {
            double f = w > 0 && h > 0 ? std::min(w / s.width(), h / s.height())
                       : w > 0        ? w / s.width()
                                      : h / s.height();
            w = s.width() * f;
            h = s.height() * f;
        } else if (w <= 0 || h <= 0)
            return Result<QSize>::failure(ErrorCode::InvalidParameter, "Both dimensions required");
    }
    if (o.preventUpscaling) {
        if (o.keepAspectRatio || o.mode != ResizeMode::ByPixels) {
            double f = std::min({1., double(s.width()) / w, double(s.height()) / h});
            w *= f;
            h *= f;
        } else {
            w = std::min(w, double(s.width()));
            h = std::min(h, double(s.height()));
        }
    }
    if (!std::isfinite(w) || !std::isfinite(h) || w > 32768 || h > 32768 || w * h > 134217728.)
        return Result<QSize>::failure(ErrorCode::ImageDimensionsTooLarge,
                                      "Target size exceeds limits");
    return Result<QSize>::success(
        QSize(std::max(1, int(std::lround(w))), std::max(1, int(std::lround(h)))));
}
namespace {
struct Tap {
    int i;
    double w;
};
double kernel(double x, InterpolationMethod m) {
    x = std::abs(x);
    if (m == InterpolationMethod::Bilinear)
        return std::max(0., 1 - x);
    if (m == InterpolationMethod::Bicubic) {
        if (x < 1)
            return (1.5 * x - 2.5) * x * x + 1;
        if (x < 2)
            return ((-.5 * x + 2.5) * x - 4) * x + 2;
        return 0;
    }
    if (x < 1e-12)
        return 1;
    if (x >= 3)
        return 0;
    return std::sin(std::numbers::pi * x) * std::sin(std::numbers::pi * x / 3) /
           (std::numbers::pi * std::numbers::pi * x * x / 3);
}
std::vector<std::vector<Tap>> weights(int src, int dst, InterpolationMethod m) {
    std::vector<std::vector<Tap>> out(dst);
    double scale = double(src) / dst, stretch = std::max(1., scale),
           radius = (m == InterpolationMethod::Bicubic    ? 2
                     : m == InterpolationMethod::Bilinear ? 1
                                                          : 3) *
                    stretch;
    for (int d = 0; d < dst; ++d) {
        double c = (d + .5) * scale - .5, sum = 0;
        for (int i = int(std::ceil(c - radius)); i <= int(std::floor(c + radius)); ++i) {
            double w = kernel((c - i) / stretch, m);
            if (w != 0) {
                out[d].push_back({std::clamp(i, 0, src - 1), w});
                sum += w;
            }
        }
        for (auto &t : out[d])
            t.w /= sum;
    }
    return out;
}
} // namespace
Result<QImage> ResizeOp::resize(const QImage &source, const ResizeOptions &o) {
    auto sz = computeTargetSize(source.size(), o);
    if (!sz.isOk())
        return Result<QImage>::failure(sz.error.code, sz.error.message);
    QSize d = *sz.value;
    if (d == source.size())
        return Result<QImage>::success(source);
    if (o.interpolation == InterpolationMethod::Nearest)
        return Result<QImage>::success(
            source.scaled(d, Qt::IgnoreAspectRatio, Qt::FastTransformation));
    // Floating-point premultiplied RGBA prevents transparent RGB halos; 16-bit input is retained.
    QImage src = source.convertToFormat(QImage::Format_RGBA32FPx4_Premultiplied);
    // Choose pass order to minimize the intermediate allocation for extreme aspect changes.
    bool horizontalFirst = 1LL * d.width() * src.height() <= 1LL * src.width() * d.height();
    QSize intermediate =
        horizontalFirst ? QSize(d.width(), src.height()) : QSize(src.width(), d.height());
    QImage tmp(intermediate, QImage::Format_RGBA32FPx4_Premultiplied);
    QImage out(d, QImage::Format_RGBA32FPx4_Premultiplied);
    if (src.isNull() || tmp.isNull() || out.isNull())
        return Result<QImage>::failure(ErrorCode::InternalError, "Resampling allocation failed");
    auto wx = weights(src.width(), d.width(), o.interpolation),
         wy = weights(src.height(), d.height(), o.interpolation);
    auto horizontal = [&](const QImage &input, QImage &output) {
        for (int y = 0; y < output.height(); ++y) {
            auto a = reinterpret_cast<const float *>(input.constScanLine(y));
            auto b = reinterpret_cast<float *>(output.scanLine(y));
            for (int x = 0; x < output.width(); ++x)
                for (int c = 0; c < 4; ++c) {
                    double v = 0;
                    for (auto tap : wx[x])
                        v += a[tap.i * 4 + c] * tap.w;
                    b[x * 4 + c] = float(v);
                }
        }
    };
    auto vertical = [&](const QImage &input, QImage &output) {
        for (int y = 0; y < output.height(); ++y) {
            auto b = reinterpret_cast<float *>(output.scanLine(y));
            for (int x = 0; x < output.width(); ++x) {
                double v[4]{};
                for (auto tap : wy[y]) {
                    auto a = reinterpret_cast<const float *>(input.constScanLine(tap.i));
                    for (int c = 0; c < 4; ++c)
                        v[c] += a[x * 4 + c] * tap.w;
                }
                for (int c = 0; c < 4; ++c)
                    b[x * 4 + c] = float(v[c]);
            }
        }
    };
    if (horizontalFirst) {
        horizontal(src, tmp);
        vertical(tmp, out);
    } else {
        vertical(src, tmp);
        horizontal(tmp, out);
    }
    for (int y = 0; y < out.height(); ++y) {
        auto row = reinterpret_cast<float *>(out.scanLine(y));
        for (int x = 0; x < out.width(); ++x) {
            float alpha = std::clamp(row[x * 4 + 3], 0.f, 1.f);
            for (int c = 0; c < 3; ++c)
                row[x * 4 + c] = std::clamp(row[x * 4 + c], 0.f, alpha);
            row[x * 4 + 3] = alpha;
        }
    }
    out.setColorSpace(source.colorSpace());
    out.setDotsPerMeterX(source.dotsPerMeterX());
    out.setDotsPerMeterY(source.dotsPerMeterY());
    for (const auto &k : source.textKeys())
        out.setText(k, source.text(k));
    auto fmt = source.format();
    if (source.depth() < 24)
        fmt = QImage::Format_ARGB32;
    return Result<QImage>::success(out.convertToFormat(fmt));
}
} // namespace photo26::core
