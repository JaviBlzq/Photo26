#include "palette_compare_bg_op.hpp"
#include <QtCore/QBuffer>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtGui/QPainter>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>
#include <random>
#include <queue>
namespace photo26::core {
namespace {
using V = std::array<double, 3>;
double dist(const V &a, const V &b) {
    double d = 0;
    for (int c = 0; c < 3; ++c)
        d += (a[c] - b[c]) * (a[c] - b[c]);
    return d;
}
V rgb(QRgb p) { return {double(qRed(p)), double(qGreen(p)), double(qBlue(p))}; }
// CIE L*a*b*, D65 white; distances normalized to a practical 100 Delta-E range.
V lab(QRgb p) {
    auto f = [](double v) {
        v /= 255;
        return v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4);
    };
    double r = f(qRed(p)), g = f(qGreen(p)), b = f(qBlue(p));
    auto t = [](double v) { return v > .008856 ? std::cbrt(v) : 7.787 * v + 16. / 116; };
    double x = t((.4124564 * r + .3575761 * g + .1804375 * b) / .95047),
           y = t(.2126729 * r + .7151522 * g + .072175 * b),
           z = t((.0193339 * r + .119192 * g + .9503041 * b) / 1.08883);
    return {116 * y - 16, 500 * (x - y), 200 * (y - z)};
}
} // namespace
Result<std::vector<PaletteColor>> PaletteExtractorOp::extractDominantColors(const QImage &source,
                                                                            int count) {
    if (source.isNull() || count < 1 || count > 256)
        return Result<std::vector<PaletteColor>>::failure(ErrorCode::InvalidParameter,
                                                          "Invalid palette input");
    QImage img = source.convertToFormat(QImage::Format_ARGB32);
    std::vector<V> samples;
    std::vector<double> alpha;
    std::uint64_t pixels = std::uint64_t(img.width()) * img.height(),
                  step = std::max<std::uint64_t>(1, (pixels + 65535) / 65536);
    for (std::uint64_t n = 0; n < pixels; n += step) {
        auto p = reinterpret_cast<const QRgb *>(
            img.constScanLine(int(n / img.width())))[n % img.width()];
        if (qAlpha(p)) {
            samples.push_back(rgb(p));
            alpha.push_back(qAlpha(p) / 255.);
        }
    }
    if (samples.empty())
        return Result<std::vector<PaletteColor>>::failure(ErrorCode::InvalidParameter,
                                                          "Image is fully transparent");
    count = std::min(count, int(samples.size()));
    std::mt19937 rng(26);
    std::vector<V> centers{samples[rng() % samples.size()]};
    std::vector<double> distances(samples.size(), std::numeric_limits<double>::max());
    while (int(centers.size()) < count) {
        double total = 0;
        for (std::size_t i = 0; i < samples.size(); ++i) {
            distances[i] = std::min(distances[i], dist(samples[i], centers.back()));
            total += distances[i] * alpha[i];
        }
        if (total < 1e-9)
            break;
        double pick = (double(rng()) / (double(rng.max()) + 1)) * total;
        std::size_t chosen = samples.size() - 1;
        for (std::size_t i = 0; i < samples.size(); ++i) {
            pick -= distances[i] * alpha[i];
            if (pick <= 0) {
                chosen = i;
                break;
            }
        }
        centers.push_back(samples[chosen]);
    }
    std::vector<double> weights(centers.size());
    for (int iteration = 0; iteration < 50; ++iteration) {
        std::vector<V> sums(centers.size(), V{});
        std::fill(weights.begin(), weights.end(), 0);
        for (std::size_t i = 0; i < samples.size(); ++i) {
            std::size_t best = 0;
            for (std::size_t j = 1; j < centers.size(); ++j)
                if (dist(samples[i], centers[j]) < dist(samples[i], centers[best]))
                    best = j;
            weights[best] += alpha[i];
            for (int c = 0; c < 3; ++c)
                sums[best][c] += samples[i][c] * alpha[i];
        }
        double movement = 0;
        for (std::size_t j = 0; j < centers.size(); ++j)
            if (weights[j] > 0) {
                for (auto &v : sums[j])
                    v /= weights[j];
                movement += dist(sums[j], centers[j]);
                centers[j] = sums[j];
            }
        if (movement < .001)
            break;
    }
    // Reassign to final centers so reported dominance corresponds to returned colors.
    std::fill(weights.begin(), weights.end(), 0);
    for (std::size_t i = 0; i < samples.size(); ++i) {
        std::size_t best = 0;
        for (std::size_t j = 1; j < centers.size(); ++j)
            if (dist(samples[i], centers[j]) < dist(samples[i], centers[best]))
                best = j;
        weights[best] += alpha[i];
    }
    double total = std::accumulate(weights.begin(), weights.end(), 0.);
    std::vector<PaletteColor> out;
    for (std::size_t j = 0; j < centers.size(); ++j)
        if (weights[j] > 0) {
            QColor c(int(std::lround(centers[j][0])), int(std::lround(centers[j][1])),
                     int(std::lround(centers[j][2])));
            out.push_back(
                {c, c.name(QColor::HexRgb), c.red(), c.green(), c.blue(), weights[j] / total});
        }
    std::stable_sort(out.begin(), out.end(),
                     [](auto &a, auto &b) { return a.dominanceRatio > b.dominanceRatio; });
    return Result<std::vector<PaletteColor>>::success(std::move(out));
}
Result<QImage> PaletteExtractorOp::renderPaletteSwatch(const std::vector<PaletteColor> &palette,
                                                       int w, int h) {
    if (palette.empty() || w <= 0 || h <= 0 || w > 32768 || h > 32768 || 1LL * w * h > 134217728)
        return Result<QImage>::failure(ErrorCode::InvalidParameter, "Invalid swatch");
    QImage out(w, h, QImage::Format_ARGB32);
    if (out.isNull())
        return Result<QImage>::failure(ErrorCode::InternalError, "Allocation failed");
    QPainter p(&out);
    for (std::size_t i = 0; i < palette.size(); ++i) {
        int x = int(i * w / palette.size()), right = int((i + 1) * w / palette.size());
        p.fillRect(QRect(x, 0, right - x, h), palette[i].color);
    }
    p.end();
    return Result<QImage>::success(out);
}
Result<QByteArray> PaletteExtractorOp::exportPalette(const std::vector<PaletteColor> &palette,
                                                     PaletteExportFormat fmt, const QString &name) {
    if (palette.empty())
        return Result<QByteArray>::failure(ErrorCode::InvalidParameter, "Empty palette");
    if (fmt == PaletteExportFormat::SwatchPng) {
        auto img = renderPaletteSwatch(palette);
        if (!img.isOk())
            return Result<QByteArray>::failure(img.error.code, img.error.message);
        QByteArray bytes;
        QBuffer b(&bytes);
        b.open(QIODevice::WriteOnly);
        if (!img.value->save(&b, "PNG"))
            return Result<QByteArray>::failure(ErrorCode::FileWriteError, "PNG encoding failed");
        return Result<QByteArray>::success(bytes);
    }
    if (fmt == PaletteExportFormat::Json) {
        QJsonArray colors;
        for (auto &c : palette)
            colors.append(QJsonObject{{"hex", c.hex},
                                      {"r", c.r},
                                      {"g", c.g},
                                      {"b", c.b},
                                      {"dominanceRatio", c.dominanceRatio}});
        return Result<QByteArray>::success(
            QJsonDocument(QJsonObject{{"name", name}, {"colors", colors}}).toJson());
    }
    QString text =
        fmt == PaletteExportFormat::CssVariables
            ? ":root {\n"
            : "GIMP Palette\nName: " + QString(name).replace('\n', ' ').replace('\r', ' ') +
                  "\nColumns: 6\n#\n";
    for (std::size_t i = 0; i < palette.size(); ++i) {
        auto &c = palette[i];
        if (fmt == PaletteExportFormat::CssVariables)
            text += QString("  --color-%1: %2;\n").arg(i + 1).arg(c.color.name());
        else
            text += QString("%1 %2 %3\t%4\n").arg(c.r).arg(c.g).arg(c.b).arg(c.hex);
    }
    if (fmt == PaletteExportFormat::CssVariables)
        text += "}\n";
    return Result<QByteArray>::success(text.toUtf8());
}
Result<ComparisonMetrics> ImageCompareOp::compare(const QImage &a, const QImage &b,
                                                  ComparisonMode mode, double fraction,
                                                  std::uint64_t ba, std::uint64_t bb) {
    if (a.isNull() || b.isNull() || !std::isfinite(fraction) || fraction < 0 || fraction > 1)
        return Result<ComparisonMetrics>::failure(ErrorCode::InvalidParameter,
                                                  "Invalid comparison input");
    // Different sizes are placed at the top-left on a shared transparent canvas; no resampling
    // hides differences.
    int w = std::max(a.width(), b.width()), h = std::max(a.height(), b.height());
    if (1LL * w * h > 134217728 || w > 32768 || h > 32768 ||
        (mode == ComparisonMode::SideBySide && w > 16384))
        return Result<ComparisonMetrics>::failure(ErrorCode::ImageDimensionsTooLarge,
                                                  "Comparison too large");
    QImage aa(w, h, QImage::Format_ARGB32), ib(w, h, QImage::Format_ARGB32);
    aa.fill(Qt::transparent);
    ib.fill(Qt::transparent);
    {
        QPainter p(&aa);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.drawImage(0, 0, a);
    }
    {
        QPainter p(&ib);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.drawImage(0, 0, b);
    }
    QImage diff(w, h, QImage::Format_ARGB32);
    double absSum = 0, squares = 0;
    std::uint64_t changed = 0;
    for (int y = 0; y < h; ++y) {
        auto ar = reinterpret_cast<const QRgb *>(aa.constScanLine(y)),
             br = reinterpret_cast<const QRgb *>(ib.constScanLine(y));
        auto dr = reinterpret_cast<QRgb *>(diff.scanLine(y));
        for (int x = 0; x < w; ++x) {
            int da = std::abs(qAlpha(ar[x]) - qAlpha(br[x]));
            int d[3] = {std::abs(qRed(qPremultiply(ar[x])) - qRed(qPremultiply(br[x]))),
                        std::abs(qGreen(qPremultiply(ar[x])) - qGreen(qPremultiply(br[x]))),
                        std::abs(qBlue(qPremultiply(ar[x])) - qBlue(qPremultiply(br[x])))};
            bool any = da != 0;
            for (int c : d) {
                absSum += c;
                squares += c * c;
                any |= c != 0;
            }
            absSum += da;
            squares += da * da;
            changed += any;
            dr[x] = qRgb(std::max(d[0], da), std::max(d[1], da), std::max(d[2], da));
        }
    }
    ComparisonMetrics m;
    m.sizeA = a.size();
    m.sizeB = b.size();
    m.bytesA = ba;
    m.bytesB = bb;
    double components = 4. * w * h;
    m.meanAbsoluteError = absSum / components;
    m.rmse = std::sqrt(squares / components);
    m.psnrDb =
        m.rmse == 0 ? std::numeric_limits<double>::infinity() : 20 * std::log10(255 / m.rmse);
    m.diffPixelPercent = 100. * changed / (double(w) * h);
    if (mode == ComparisonMode::VisualDifference)
        m.renderedView = diff;
    else if (mode == ComparisonMode::SideBySide) {
        m.renderedView = QImage(w * 2, h, QImage::Format_ARGB32);
        m.renderedView.fill(Qt::transparent);
        QPainter p(&m.renderedView);
        p.drawImage(0, 0, aa);
        p.drawImage(w, 0, ib);
    } else {
        m.renderedView = aa;
        QPainter p(&m.renderedView);
        if (mode == ComparisonMode::SplitSlider) {
            p.setCompositionMode(QPainter::CompositionMode_Source);
            p.setClipRect(QRect(int(w * fraction), 0, w - int(w * fraction), h));
        } else
            p.setOpacity(fraction);
        p.drawImage(0, 0, ib);
    }
    return Result<ComparisonMetrics>::success(m);
}
bool BackgroundRemovalOp::isOnnxRuntimeAvailable() { return false; }
Result<QImage> BackgroundRemovalOp::removeBackground(const QImage &source,
                                                     const BgRemovalOptions &o) {
    if (source.isNull() || !std::isfinite(o.tolerance) || o.tolerance < .01 || o.tolerance > .8 ||
        !std::isfinite(o.featherRadius) || o.featherRadius < 0 || o.featherRadius > 100)
        return Result<QImage>::failure(ErrorCode::InvalidParameter,
                                       "Invalid background removal options");
    if (!o.optionalOnnxModelPath.isEmpty())
        return Result<QImage>::failure(
            ErrorCode::UnsupportedFormat,
            "ONNX runtime unavailable; omit model path to use local segmentation");
    QImage img = source.convertToFormat(QImage::Format_ARGB32);
    int w = img.width(), h = img.height();
    std::size_t n = std::size_t(w) * h;
    if (n > 134217728)
        return Result<QImage>::failure(ErrorCode::ImageDimensionsTooLarge, "Image too large");
    std::vector<V> border;
    int stride = std::max(1, (2 * w + 2 * h) / 2048);
    for (int x = 0; x < w; x += stride) {
        border.push_back(lab(img.pixel(x, 0)));
        border.push_back(lab(img.pixel(x, h - 1)));
    }
    for (int y = 0; y < h; y += stride) {
        border.push_back(lab(img.pixel(0, y)));
        border.push_back(lab(img.pixel(w - 1, y)));
    }
    // Quantized border modes retain multiple backdrop colors while rejecting rare foreground border
    // outliers.
    struct Mode {
        V mean{};
        int count{0};
    };
    std::vector<Mode> modes;
    for (auto &v : border) {
        std::size_t best = modes.size();
        for (std::size_t j = 0; j < modes.size(); ++j)
            if (dist(v, modes[j].mean) < 100) {
                best = j;
                break;
            }
        if (best == modes.size())
            modes.push_back({v, 1});
        else {
            auto &m = modes[best];
            for (int c = 0; c < 3; ++c)
                m.mean[c] = (m.mean[c] * m.count + v[c]) / (m.count + 1);
            ++m.count;
        }
    }
    std::stable_sort(modes.begin(), modes.end(),
                     [](auto &a, auto &b) { return a.count > b.count; });
    if (modes.size() > 12)
        modes.resize(12);
    int minimum = std::max(2, int(border.size() / 50));
    modes.erase(
        std::remove_if(modes.begin(), modes.end(), [&](auto &m) { return m.count < minimum; }),
        modes.end());
    if (modes.empty())
        modes.push_back({border.front(), 1});
    V key = lab(o.keyColor.rgba());
    std::vector<float> alpha(n, 1);
    double threshold = o.tolerance * 100;
    auto distanceAt = [&](int x, int y) {
        QRgb p = img.pixel(x, y);
        if (qAlpha(p) == 0)
            return 0.;
        V v = lab(p);
        if (o.method == BgRemovalMethod::ChromaKeyColor)
            return std::sqrt(dist(v, key));
        double d = 1e9;
        for (auto &m : modes)
            d = std::min(d, dist(v, m.mean));
        return std::sqrt(d);
    };
    if (o.method == BgRemovalMethod::AutoBorderFloodAndGrabCut) {
        std::vector<unsigned char> seen(n);
        std::queue<std::size_t> q;
        auto seed = [&](int x, int y) {
            auto i = std::size_t(y) * w + x;
            if (!seen[i] && distanceAt(x, y) <= threshold) {
                seen[i] = 1;
                q.push(i);
            }
        };
        for (int x = 0; x < w; ++x) {
            seed(x, 0);
            seed(x, h - 1);
        }
        for (int y = 0; y < h; ++y) {
            seed(0, y);
            seed(w - 1, y);
        }
        while (!q.empty()) {
            auto i = q.front();
            q.pop();
            alpha[i] = 0;
            int x = int(i % w), y = int(i / w);
            if (x)
                seed(x - 1, y);
            if (x + 1 < w)
                seed(x + 1, y);
            if (y)
                seed(x, y - 1);
            if (y + 1 < h)
                seed(x, y + 1);
        }
    } else
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x) {
                double d;
                if (o.method == BgRemovalMethod::LuminanceThreshold) {
                    auto p = img.pixel(x, y);
                    double l = (.2126 * qRed(p) + .7152 * qGreen(p) + .0722 * qBlue(p)) / 255.,
                           k = .2126 * o.keyColor.redF() + .7152 * o.keyColor.greenF() +
                               .0722 * o.keyColor.blueF();
                    d = std::abs(l - k) * 100;
                } else
                    d = distanceAt(x, y);
                double soft = std::max(.5, threshold * .15);
                alpha[std::size_t(y) * w + x] =
                    float(std::clamp((d - threshold + soft) / (2 * soft), 0., 1.));
            }
    // Separable Gaussian boundary matting; only the local alpha transition is softened.
    if (o.featherRadius > 0) {
        double sigma = std::max(.3, o.featherRadius);
        int radius = int(std::ceil(3 * sigma));
        std::vector<double> k(2 * radius + 1);
        double sum = 0;
        for (int j = -radius; j <= radius; ++j)
            sum += (k[j + radius] = std::exp(-j * j / (2 * sigma * sigma)));
        for (auto &v : k)
            v /= sum;
        std::vector<float> tmp(n);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x) {
                double v = 0;
                for (int j = -radius; j <= radius; ++j)
                    v += alpha[std::size_t(y) * w + std::clamp(x + j, 0, w - 1)] * k[j + radius];
                tmp[std::size_t(y) * w + x] = float(v);
            }
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x) {
                double v = 0;
                for (int j = -radius; j <= radius; ++j)
                    v += tmp[std::size_t(std::clamp(y + j, 0, h - 1)) * w + x] * k[j + radius];
                alpha[std::size_t(y) * w + x] = float(v);
            }
    }
    for (int y = 0; y < h; ++y) {
        auto row = reinterpret_cast<QRgb *>(img.scanLine(y));
        for (int x = 0; x < w; ++x) {
            auto p = row[x];
            row[x] = qRgba(qRed(p), qGreen(p), qBlue(p),
                           int(std::lround(qAlpha(p) * alpha[std::size_t(y) * w + x])));
        }
    }
    if (o.replaceWithSolidColor) {
        QImage out(img.size(), QImage::Format_ARGB32_Premultiplied);
        out.fill(o.replacementColor);
        QPainter p(&out);
        p.drawImage(0, 0, img);
        p.end();
        return Result<QImage>::success(out);
    }
    return Result<QImage>::success(img);
}
} // namespace photo26::core
