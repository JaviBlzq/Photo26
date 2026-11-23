#include "compress_web_op.hpp"
#include "resize_op.hpp"
#include <QtCore/QBuffer>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSet>
#include <QtGui/QImageWriter>
#include <QtGui/QPainter>
#include <QtGui/QColorSpace>
#include <algorithm>
#include <cmath>
namespace photo26::core {
namespace {
Result<QByteArray> encode(const QImage &source, const QString &format, int quality, bool lossless,
                          const QColor &bg) {
    QByteArray fmt = format.toLower().toLatin1();
    if (fmt == "jpeg")
        fmt = "jpg";
    if (!QImageWriter::supportedImageFormats().contains(fmt) &&
        !(fmt == "jpg" && QImageWriter::supportedImageFormats().contains("jpeg")))
        return Result<QByteArray>::failure(ErrorCode::UnsupportedFormat, "No encoder installed",
                                           format);
    if (lossless && (fmt == "jpg" || fmt == "jpeg"))
        return Result<QByteArray>::failure(ErrorCode::InvalidParameter,
                                           "JPEG does not support lossless encoding");
    QImage img = source;
    if (fmt == "jpg" || fmt == "bmp") {
        img = QImage(source.size(), QImage::Format_RGB32);
        img.fill(bg);
        QPainter p(&img);
        p.drawImage(0, 0, source);
        p.end();
        img.setColorSpace(source.colorSpace());
    }
    QByteArray bytes;
    QBuffer b(&bytes);
    b.open(QIODevice::WriteOnly);
    QImageWriter writer(&b, fmt);
    writer.setQuality(lossless && fmt == "webp" ? 100 : quality);
    if (fmt == "png")
        writer.setCompression(9);
    if (!writer.write(img))
        return Result<QByteArray>::failure(ErrorCode::FileWriteError, writer.errorString());
    return Result<QByteArray>::success(bytes);
}
} // namespace
Result<CompressionMetrics> CompressWebOp::compressInMemory(const QImage &source,
                                                           const CompressionOptions &o,
                                                           std::uint64_t orig) {
    if (source.isNull() || o.quality < 0 || o.quality > 100 || o.maxWidth < 0 || o.maxHeight < 0)
        return Result<CompressionMetrics>::failure(ErrorCode::InvalidParameter,
                                                   "Invalid compression options");
    QImage img = source;
    if (o.maxWidth || o.maxHeight) {
        ResizeOptions r;
        r.mode = ResizeMode::FitMaxDimensions;
        r.maxWidth = o.maxWidth ? o.maxWidth : source.width();
        r.maxHeight = o.maxHeight ? o.maxHeight : source.height();
        r.preventUpscaling = true;
        auto rr = ResizeOp::resize(img, r);
        if (!rr.isOk())
            return Result<CompressionMetrics>::failure(rr.error.code, rr.error.message);
        img = *rr.value;
    }
    CompressionMetrics m;
    m.originalBytes = orig;
    Result<QByteArray> best;
    for (int attempt = 0; attempt < 24; ++attempt) {
        int chosen = o.quality;
        best = encode(img, o.format, chosen, o.lossless, o.backgroundColor);
        if (!best.isOk())
            return Result<CompressionMetrics>::failure(best.error.code, best.error.message,
                                                       best.error.detail);
        if (o.targetMaxBytes) {
            auto low = encode(img, o.format, 5, o.lossless, o.backgroundColor);
            if (!low.isOk())
                return Result<CompressionMetrics>::failure(low.error.code, low.error.message);
            best = low;
            chosen = 5;
            int lo = 5, hi = 100;
            while (lo <= hi) {
                int mid = (lo + hi) / 2;
                auto trial = encode(img, o.format, mid, o.lossless, o.backgroundColor);
                if (!trial.isOk())
                    return Result<CompressionMetrics>::failure(trial.error.code,
                                                               trial.error.message);
                if (std::uint64_t(trial.value->size()) <= o.targetMaxBytes) {
                    chosen = mid;
                    best = std::move(trial);
                    lo = mid + 1;
                } else
                    hi = mid - 1;
            }
        }
        m.chosenQuality = o.lossless && o.format.toLower() == "webp" ? 100 : chosen;
        if (!o.targetMaxBytes || std::uint64_t(best.value->size()) <= o.targetMaxBytes ||
            img.size() == QSize(1, 1) || attempt == 23)
            break;
        double factor =
            std::clamp(std::sqrt(double(o.targetMaxBytes) / best.value->size()) * .95, .1, .95);
        ResizeOptions r;
        r.targetWidth = std::max(1, int(img.width() * factor));
        r.targetHeight = std::max(1, int(img.height() * factor));
        r.keepAspectRatio = false;
        auto rr = ResizeOp::resize(img, r);
        if (!rr.isOk())
            return Result<CompressionMetrics>::failure(rr.error.code, rr.error.message);
        img = *rr.value;
    }
    m.encodedBytes = *best.value;
    m.compressedBytes = m.encodedBytes.size();
    m.outputDimensions = img.size();
    m.savingsPercent = orig ? 100. * (1. - double(m.compressedBytes) / orig) : 0;
    return Result<CompressionMetrics>::success(std::move(m));
}
Result<std::vector<WebVariantResult>>
CompressWebOp::generateWebVariants(const QImage &source, const QString &base,
                                   const WebOptimizerOptions &o) {
    if (source.isNull() || o.widths.empty())
        return Result<std::vector<WebVariantResult>>::failure(ErrorCode::InvalidParameter,
                                                              "No image or variant widths");
    std::vector<WebVariantResult> out;
    QSet<QString> names;
    for (int w : o.widths) {
        if (w <= 0)
            return Result<std::vector<WebVariantResult>>::failure(ErrorCode::InvalidParameter,
                                                                  "Invalid width");
        ResizeOptions r;
        r.targetWidth = w;
        r.keepAspectRatio = true;
        r.preventUpscaling = o.preventUpscaling;
        auto resized = ResizeOp::resize(source, r);
        if (!resized.isOk())
            return Result<std::vector<WebVariantResult>>::failure(resized.error.code,
                                                                  resized.error.message);
        QImage img = *resized.value;
        // Width-only variants always preserve aspect ratio; there is no independent height in this
        // API.
        if (o.stripMetadata) {
            QImage clean(img.size(), img.format());
            if (clean.isNull())
                return Result<std::vector<WebVariantResult>>::failure(ErrorCode::InternalError,
                                                                      "Allocation failed");
            QPainter p(&clean);
            p.setCompositionMode(QPainter::CompositionMode_Source);
            p.drawImage(0, 0, img);
            p.end();
            clean.setColorSpace(img.colorSpace());
            img = clean;
        }
        CompressionOptions c;
        c.format = o.format;
        c.quality = o.quality;
        c.lossless = o.lossless;
        auto enc = compressInMemory(img, c);
        if (!enc.isOk())
            return Result<std::vector<WebVariantResult>>::failure(enc.error.code,
                                                                  enc.error.message);
        QString name = o.namingPattern;
        name.replace("{name}", QFileInfo(base).completeBaseName())
            .replace("{width}", QString::number(w))
            .replace("{ext}", o.format.toLower());
        if (name.isEmpty() || name == "." || name == ".." || name.contains('/') ||
            name.contains('\\') || name.contains(':') || names.contains(name.toCaseFolded()))
            return Result<std::vector<WebVariantResult>>::failure(
                ErrorCode::ConflictDetected, "Unsafe or duplicate output name", name);
        names.insert(name.toCaseFolded());
        out.push_back({w, img.size(), name, enc.value->encodedBytes, enc.value->compressedBytes});
    }
    return Result<std::vector<WebVariantResult>>::success(std::move(out));
}
Result<QStringList> CompressWebOp::saveWebVariantsToFolder(const QImage &s, const QString &base,
                                                           const QString &dir,
                                                           const WebOptimizerOptions &o,
                                                           bool overwrite) {
    auto variants = generateWebVariants(s, base, o);
    if (!variants.isOk())
        return Result<QStringList>::failure(variants.error.code, variants.error.message);
    QStringList paths;
    for (auto &v : *variants.value) {
        QString path = QDir(dir).filePath(v.fileName);
        if (!overwrite && QFileInfo::exists(path))
            return Result<QStringList>::failure(ErrorCode::ConflictDetected,
                                                "Output already exists", path);
        paths << path;
    }
    for (std::size_t i = 0; i < variants.value->size(); ++i) {
        auto st = SafeFileWriter::writeBytes(paths[int(i)], (*variants.value)[i].encodedBytes,
                                             {overwrite, true});
        if (!st.isOk())
            return Result<QStringList>::failure(st.error.code, st.error.message);
    }
    return Result<QStringList>::success(paths);
}
} // namespace photo26::core
