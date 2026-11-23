#include "batch_rename_op.hpp"
#include "compress_web_op.hpp"
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QSet>
#include <QtCore/QRegularExpression>
#include <QtCore/QThreadPool>
#include <QtConcurrent/QtConcurrentRun>
#include <QtGui/QImageReader>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <QtGui/QColorSpace>
#include <QtCore/QBuffer>
#include <exception>
namespace photo26::core {
namespace {
QString pathKey(const QString &p) {
    return QDir::cleanPath(QFileInfo(p).absoluteFilePath()).toCaseFolded();
}
bool safeName(const QString &n) {
    return !n.isEmpty() && n != "." && n != ".." &&
           !n.contains(QRegularExpression(R"([<>:"/\\|?*\x00-\x1f])")) && !n.endsWith('.') &&
           !n.endsWith(' ') &&
           !QRegularExpression("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\\.|$)",
                               QRegularExpression::CaseInsensitiveOption)
                .match(n)
                .hasMatch();
}
} // namespace
std::vector<RenamePreviewItem> MassRenameOp::previewRename(const QStringList &paths,
                                                           const MassRenameOptions &o,
                                                           const QString &dir) {
    std::vector<RenamePreviewItem> out;
    QHash<QString, int> destinations;
    for (int i = 0; i < paths.size(); ++i) {
        QFileInfo f(paths[i]);
        QImageReader reader(paths[i]);
        QSize size = reader.size();
        QString ext = o.forceExtension.isEmpty() ? f.suffix() : o.forceExtension;
        if (ext.startsWith('.'))
            ext.remove(0, 1);
        QString stem = f.completeBaseName();
        if (!o.findText.isEmpty())
            stem.replace(o.findText, o.replaceText);
        QString name = o.pattern;
        name.replace("{name}", stem)
            .replace("{num}", QString::number(static_cast<qint64>(o.startNumber) +
                                              static_cast<qint64>(i) * o.numberStep)
                                  .rightJustified(std::clamp(o.zeroPadding, 0, 32), '0'))
            .replace("{width}", QString::number(std::max(0, size.width())))
            .replace("{height}", QString::number(std::max(0, size.height())))
            .replace("{date}", f.lastModified().date().toString("yyyy-MM-dd"))
            .replace("{ext}", ext);
        bool includesExt = o.pattern.contains("{ext}");
        name = o.prefix + name + o.suffix;
        if (!includesExt && !ext.isEmpty())
            name += '.' + ext;
        QString dest = QDir(dir.isEmpty() ? f.absolutePath() : dir).absoluteFilePath(name);
        RenamePreviewItem item{f.absoluteFilePath(), f.fileName(), name, dest, false, {}};
        if (!f.isFile()) {
            item.hasConflict = true;
            item.conflictReason = "Source file does not exist";
        }
        if (!safeName(name)) {
            item.hasConflict = true;
            item.conflictReason = "Invalid output filename";
        }
        QString key = pathKey(dest);
        if (destinations.contains(key)) {
            item.hasConflict = true;
            item.conflictReason = "Duplicate batch destination";
            auto &prior = out[destinations[key]];
            prior.hasConflict = true;
            prior.conflictReason = item.conflictReason;
        } else
            destinations.insert(key, int(out.size()));
        if (QFileInfo::exists(dest) && pathKey(dest) != pathKey(item.originalPath)) {
            item.hasConflict = true;
            item.conflictReason = "Destination already exists";
        }
        out.push_back(item);
    }
    return out;
}
StatusResult MassRenameOp::executeRenameOrCopy(const std::vector<RenamePreviewItem> &items,
                                               bool copy, bool overwrite) {
    QSet<QString> destinations, sources;
    for (auto &i : items)
        sources.insert(pathKey(i.originalPath));
    for (auto &i : items) {
        auto key = pathKey(i.proposedPath);
        if (!safeName(QFileInfo(i.proposedPath).fileName()) || destinations.contains(key) ||
            (!overwrite && i.hasConflict))
            return StatusResult::failure(ErrorCode::ConflictDetected,
                                         "Invalid or conflicting destination", i.proposedPath);
        if (sources.contains(key) && key != pathKey(i.originalPath))
            return StatusResult::failure(ErrorCode::ConflictDetected,
                                         "Destination is another batch source", i.proposedPath);
        if (!QFileInfo(i.originalPath).isFile())
            return StatusResult::failure(ErrorCode::FileNotFound, "Source missing", i.originalPath);
        if (!overwrite && QFileInfo::exists(i.proposedPath) && key != pathKey(i.originalPath))
            return StatusResult::failure(ErrorCode::ConflictDetected, "Output exists",
                                         i.proposedPath);
        destinations.insert(key);
    }
    for (auto &i : items) {
        if (pathKey(i.originalPath) == pathKey(i.proposedPath))
            continue;
        auto st = SafeFileWriter::writeWithCallback(
            i.proposedPath,
            [&](const QString &temp) {
                QFile in(i.originalPath), out(temp);
                if (!in.open(QIODevice::ReadOnly) ||
                    !out.open(QIODevice::WriteOnly | QIODevice::Truncate))
                    return StatusResult::failure(ErrorCode::FileWriteError,
                                                 "Cannot open copy files");
                while (!in.atEnd()) {
                    QByteArray bytes = in.read(1024 * 1024);
                    if (bytes.isEmpty() && in.error() != QFileDevice::NoError)
                        return StatusResult::failure(ErrorCode::FileReadError, in.errorString());
                    if (out.write(bytes) != bytes.size())
                        return StatusResult::failure(ErrorCode::FileWriteError, out.errorString());
                }
                if (!out.flush())
                    return StatusResult::failure(ErrorCode::FileWriteError, out.errorString());
                return StatusResult::success();
            },
            {overwrite, true});
        if (!st.isOk())
            return st;
        if (!copy && !QFile::remove(i.originalPath))
            return StatusResult::failure(ErrorCode::FileWriteError,
                                         "Copied successfully but could not remove source",
                                         i.originalPath);
    }
    return StatusResult::success();
}
BatchSummary BatchProcessor::runBatch(const QStringList &paths, const QString &dir,
                                      const BatchPreset &preset, bool overwrite,
                                      const CancellationToken &token,
                                      const ProgressCallback &progress, int threads,
                                      const CustomBatchImageSaver &saver) {
    BatchSummary summary;
    summary.totalItems = paths.size();
    summary.itemResults.resize(paths.size());
    MassRenameOptions rename = preset.renameOptions;
    rename.forceExtension = preset.outputFormat;
    auto preview = MassRenameOp::previewRename(paths, rename, dir);
    QHash<QString, int> seen;
    QSet<QString> inputKeys;
    for (const auto &path : paths)
        inputKeys.insert(pathKey(path));
    for (std::size_t i = 0; i < preview.size(); ++i) {
        auto &r = summary.itemResults[i];
        r.sourcePath = preview[i].originalPath;
        r.outputPath = preview[i].proposedPath;
        r.inputBytes = std::max<qint64>(0, QFileInfo(r.sourcePath).size());
        auto key = pathKey(r.outputPath);
        if (inputKeys.contains(key))
            r.errorMessage = "Refusing to overwrite a source image";
        else if (seen.contains(key)) {
            r.errorMessage = "Duplicate batch destination";
            summary.itemResults[seen[key]].errorMessage = r.errorMessage;
        } else if (!safeName(preview[i].proposedName))
            r.errorMessage = "Invalid output filename";
        else if (!overwrite && QFileInfo::exists(r.outputPath))
            r.errorMessage = "Destination already exists";
        seen.insert(key, int(i));
    }
    QThreadPool pool;
    pool.setMaxThreadCount(
        std::clamp(threads > 0 ? threads : std::min(4, QThread::idealThreadCount()), 1, 64));
    std::atomic<int> next{0}, completed{0};
    std::mutex callbacks;
    std::vector<QFuture<void>> futures;
    for (int worker = 0; worker < std::min(pool.maxThreadCount(), int(paths.size())); ++worker)
        futures.push_back(QtConcurrent::run(&pool, [&] {
            for (;;) {
                int index = next.fetch_add(1);
                if (index >= paths.size())
                    break;
                auto &r = summary.itemResults[index];
                try {
                    if (token.isCancelled()) {
                        r.skipped = true;
                        r.errorMessage = "Cancelled";
                    } else if (r.errorMessage.isEmpty()) {
                        QImageReader reader(r.sourcePath);
                        reader.setAutoTransform(true);
                        QSize size = reader.size();
                        if (size.width() > 32768 || size.height() > 32768 ||
                            1LL * size.width() * size.height() > 134217728) {
                            r.errorMessage = "Input dimensions exceed limits";
                        } else {
                            QImage image = reader.read();
                            if (image.isNull())
                                r.errorMessage = reader.errorString();
                            else {
                                auto apply = [&](Result<QImage> result) {
                                    if (!result.isOk()) {
                                        r.errorMessage = result.error.message;
                                        return false;
                                    }
                                    image = std::move(*result.value);
                                    return true;
                                };
                                bool ok = true;
                                if (preset.enableResize)
                                    ok = apply(ResizeOp::resize(image, preset.resizeOptions));
                                if (ok && preset.enableColorAdjust)
                                    ok = apply(ColorAdjustOp::apply(image, preset.colorOptions));
                                if (ok && preset.enableWatermark)
                                    ok = apply(WatermarkTextOp::applyWatermark(
                                        image, preset.watermarkOptions));
                                if (ok && token.isCancelled()) {
                                    r.skipped = true;
                                    r.errorMessage = "Cancelled";
                                    ok = false;
                                }
                                if (ok) {
                                    if (preset.stripMetadata) {
                                        QImage clean(image.size(), image.format());
                                        if (clean.isNull()) {
                                            ok = false;
                                            r.errorMessage = "Allocation failed";
                                        } else {
                                            for (int y = 0; y < image.height(); ++y)
                                                std::copy_n(image.constScanLine(y),
                                                            std::min(image.bytesPerLine(),
                                                                     clean.bytesPerLine()),
                                                            clean.scanLine(y));
                                            clean.setColorSpace(image.colorSpace());
                                            image = clean;
                                        }
                                    }
                                    if (ok) {
                                        StatusResult status;
                                        if (saver)
                                            status =
                                                saver(image, r.sourcePath, r.outputPath, preset);
                                        else {
                                            CompressionOptions c;
                                            c.format = preset.outputFormat;
                                            c.quality = preset.quality;
                                            c.lossless = preset.lossless;
                                            c.backgroundColor = preset.backgroundColor;
                                            auto encoded =
                                                CompressWebOp::compressInMemory(image, c);
                                            if (!encoded.isOk())
                                                status = StatusResult::failure(
                                                    encoded.error.code, encoded.error.message);
                                            else if (token.isCancelled()) {
                                                status = StatusResult::failure(ErrorCode::Cancelled,
                                                                               "Cancelled");
                                                r.skipped = true;
                                            } else
                                                status = SafeFileWriter::writeBytes(
                                                    r.outputPath, encoded.value->encodedBytes,
                                                    {overwrite, true});
                                        }
                                        r.success = status.isOk();
                                        if (!r.success)
                                            r.errorMessage = status.error.message;
                                        else
                                            r.outputBytes =
                                                std::max<qint64>(0, QFileInfo(r.outputPath).size());
                                    }
                                }
                            }
                        }
                    }
                } catch (const std::exception &e) {
                    r.errorMessage = QString::fromUtf8(e.what());
                } catch (...) {
                    r.errorMessage = "Unhandled per-file failure";
                }
                {
                    std::lock_guard lock(callbacks);
                    int done = ++completed;
                    if (progress) {
                        try {
                            progress(done, paths.size(), r.sourcePath);
                        } catch (...) { /* User progress observers cannot abort remaining files. */
                        }
                    }
                }
            }
        }));
    for (auto &f : futures)
        f.waitForFinished();
    for (auto &r : summary.itemResults) {
        summary.totalInputBytes += r.inputBytes;
        summary.totalOutputBytes += r.outputBytes;
        if (r.success)
            ++summary.succeeded;
        else if (r.skipped)
            ++summary.cancelled;
        else
            ++summary.failed;
    }
    return summary;
}
namespace {
QJsonValue jsonColor(const QColor &c) { return c.name(QColor::HexArgb); }
QJsonObject encodeBatchPreset(const BatchPreset &v) {
    QJsonObject o;
    o["name"] = v.name;
    o["outputFormat"] = v.outputFormat;
    o["quality"] = v.quality;
    o["lossless"] = v.lossless;
    o["stripMetadata"] = v.stripMetadata;
    o["enableResize"] = v.enableResize;
    o["enableColorAdjust"] = v.enableColorAdjust;
    o["enableWatermark"] = v.enableWatermark;
    o["backgroundColor"] = jsonColor(v.backgroundColor);
    return o;
}
bool decodeBatchPreset(const QJsonObject &o, BatchPreset &v) {
    if (o.contains("name")) {
        if (!o["name"].isString())
            return false;
        v.name = o["name"].toString();
    }
    if (o.contains("outputFormat")) {
        if (!o["outputFormat"].isString())
            return false;
        v.outputFormat = o["outputFormat"].toString();
    }
    if (o.contains("quality")) {
        if (!o["quality"].isDouble())
            return false;
        if (o["quality"].toDouble() != o["quality"].toInt())
            return false;
        v.quality = o["quality"].toInt();
    }
    if (o.contains("lossless")) {
        if (!o["lossless"].isBool())
            return false;
        v.lossless = o["lossless"].toBool();
    }
    if (o.contains("stripMetadata")) {
        if (!o["stripMetadata"].isBool())
            return false;
        v.stripMetadata = o["stripMetadata"].toBool();
    }
    if (o.contains("enableResize")) {
        if (!o["enableResize"].isBool())
            return false;
        v.enableResize = o["enableResize"].toBool();
    }
    if (o.contains("enableColorAdjust")) {
        if (!o["enableColorAdjust"].isBool())
            return false;
        v.enableColorAdjust = o["enableColorAdjust"].toBool();
    }
    if (o.contains("enableWatermark")) {
        if (!o["enableWatermark"].isBool())
            return false;
        v.enableWatermark = o["enableWatermark"].toBool();
    }
    if (o.contains("backgroundColor")) {
        if (!o["backgroundColor"].isString())
            return false;
        v.backgroundColor = QColor(o["backgroundColor"].toString());
        if (!v.backgroundColor.isValid())
            return false;
    }
    return true;
}
QJsonObject encodeResizeOptions(const ResizeOptions &v) {
    QJsonObject o;
    o["mode"] = int(v.mode);
    o["targetWidth"] = v.targetWidth;
    o["targetHeight"] = v.targetHeight;
    o["percentage"] = v.percentage;
    o["maxWidth"] = v.maxWidth;
    o["maxHeight"] = v.maxHeight;
    o["keepAspectRatio"] = v.keepAspectRatio;
    o["preventUpscaling"] = v.preventUpscaling;
    o["interpolation"] = int(v.interpolation);
    return o;
}
bool decodeResizeOptions(const QJsonObject &o, ResizeOptions &v) {
    if (o.contains("mode")) {
        if (!o["mode"].isDouble())
            return false;
        if (o["mode"].toDouble() != o["mode"].toInt())
            return false;
        v.mode = static_cast<decltype(v.mode)>(o["mode"].toInt());
    }
    if (o.contains("targetWidth")) {
        if (!o["targetWidth"].isDouble())
            return false;
        if (o["targetWidth"].toDouble() != o["targetWidth"].toInt())
            return false;
        v.targetWidth = o["targetWidth"].toInt();
    }
    if (o.contains("targetHeight")) {
        if (!o["targetHeight"].isDouble())
            return false;
        if (o["targetHeight"].toDouble() != o["targetHeight"].toInt())
            return false;
        v.targetHeight = o["targetHeight"].toInt();
    }
    if (o.contains("percentage")) {
        if (!o["percentage"].isDouble())
            return false;
        v.percentage = o["percentage"].toDouble();
    }
    if (o.contains("maxWidth")) {
        if (!o["maxWidth"].isDouble())
            return false;
        if (o["maxWidth"].toDouble() != o["maxWidth"].toInt())
            return false;
        v.maxWidth = o["maxWidth"].toInt();
    }
    if (o.contains("maxHeight")) {
        if (!o["maxHeight"].isDouble())
            return false;
        if (o["maxHeight"].toDouble() != o["maxHeight"].toInt())
            return false;
        v.maxHeight = o["maxHeight"].toInt();
    }
    if (o.contains("keepAspectRatio")) {
        if (!o["keepAspectRatio"].isBool())
            return false;
        v.keepAspectRatio = o["keepAspectRatio"].toBool();
    }
    if (o.contains("preventUpscaling")) {
        if (!o["preventUpscaling"].isBool())
            return false;
        v.preventUpscaling = o["preventUpscaling"].toBool();
    }
    if (o.contains("interpolation")) {
        if (!o["interpolation"].isDouble())
            return false;
        if (o["interpolation"].toDouble() != o["interpolation"].toInt())
            return false;
        v.interpolation = static_cast<decltype(v.interpolation)>(o["interpolation"].toInt());
    }
    return true;
}
QJsonObject encodeColorAdjustOptions(const ColorAdjustOptions &v) {
    QJsonObject o;
    o["grayscale"] = v.grayscale;
    o["blackAndWhite"] = v.blackAndWhite;
    o["bwThreshold"] = v.bwThreshold;
    o["bwDither"] = v.bwDither;
    o["sepia"] = v.sepia;
    o["sepiaIntensity"] = v.sepiaIntensity;
    o["invert"] = v.invert;
    o["brightness"] = v.brightness;
    o["contrast"] = v.contrast;
    o["saturation"] = v.saturation;
    o["gamma"] = v.gamma;
    o["exposureEv"] = v.exposureEv;
    return o;
}
bool decodeColorAdjustOptions(const QJsonObject &o, ColorAdjustOptions &v) {
    if (o.contains("grayscale")) {
        if (!o["grayscale"].isBool())
            return false;
        v.grayscale = o["grayscale"].toBool();
    }
    if (o.contains("blackAndWhite")) {
        if (!o["blackAndWhite"].isBool())
            return false;
        v.blackAndWhite = o["blackAndWhite"].toBool();
    }
    if (o.contains("bwThreshold")) {
        if (!o["bwThreshold"].isDouble())
            return false;
        if (o["bwThreshold"].toDouble() != o["bwThreshold"].toInt())
            return false;
        v.bwThreshold = o["bwThreshold"].toInt();
    }
    if (o.contains("bwDither")) {
        if (!o["bwDither"].isBool())
            return false;
        v.bwDither = o["bwDither"].toBool();
    }
    if (o.contains("sepia")) {
        if (!o["sepia"].isBool())
            return false;
        v.sepia = o["sepia"].toBool();
    }
    if (o.contains("sepiaIntensity")) {
        if (!o["sepiaIntensity"].isDouble())
            return false;
        v.sepiaIntensity = o["sepiaIntensity"].toDouble();
    }
    if (o.contains("invert")) {
        if (!o["invert"].isBool())
            return false;
        v.invert = o["invert"].toBool();
    }
    if (o.contains("brightness")) {
        if (!o["brightness"].isDouble())
            return false;
        v.brightness = o["brightness"].toDouble();
    }
    if (o.contains("contrast")) {
        if (!o["contrast"].isDouble())
            return false;
        v.contrast = o["contrast"].toDouble();
    }
    if (o.contains("saturation")) {
        if (!o["saturation"].isDouble())
            return false;
        v.saturation = o["saturation"].toDouble();
    }
    if (o.contains("gamma")) {
        if (!o["gamma"].isDouble())
            return false;
        v.gamma = o["gamma"].toDouble();
    }
    if (o.contains("exposureEv")) {
        if (!o["exposureEv"].isDouble())
            return false;
        v.exposureEv = o["exposureEv"].toDouble();
    }
    return true;
}
QJsonObject encodeWatermarkOptions(const WatermarkOptions &v) {
    QJsonObject o;
    o["useImageWatermark"] = v.useImageWatermark;
    o["text"] = v.text;
    o["fontFamily"] = v.fontFamily;
    o["fontPointSize"] = v.fontPointSize;
    o["bold"] = v.bold;
    o["textColor"] = jsonColor(v.textColor);
    o["textOutline"] = v.textOutline;
    o["outlineColor"] = jsonColor(v.outlineColor);
    o["opacity"] = v.opacity;
    o["scalePercent"] = v.scalePercent;
    o["position"] = int(v.position);
    o["marginX"] = v.marginX;
    o["marginY"] = v.marginY;
    o["rotationDegrees"] = v.rotationDegrees;
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    if (!v.watermarkImage.isNull())
        v.watermarkImage.save(&buffer, "PNG");
    o["watermarkImagePng"] = QString::fromLatin1(bytes.toBase64());
    return o;
}
bool decodeWatermarkOptions(const QJsonObject &o, WatermarkOptions &v) {
    if (o.contains("useImageWatermark")) {
        if (!o["useImageWatermark"].isBool())
            return false;
        v.useImageWatermark = o["useImageWatermark"].toBool();
    }
    if (o.contains("text")) {
        if (!o["text"].isString())
            return false;
        v.text = o["text"].toString();
    }
    if (o.contains("fontFamily")) {
        if (!o["fontFamily"].isString())
            return false;
        v.fontFamily = o["fontFamily"].toString();
    }
    if (o.contains("fontPointSize")) {
        if (!o["fontPointSize"].isDouble())
            return false;
        if (o["fontPointSize"].toDouble() != o["fontPointSize"].toInt())
            return false;
        v.fontPointSize = o["fontPointSize"].toInt();
    }
    if (o.contains("bold")) {
        if (!o["bold"].isBool())
            return false;
        v.bold = o["bold"].toBool();
    }
    if (o.contains("textColor")) {
        if (!o["textColor"].isString())
            return false;
        v.textColor = QColor(o["textColor"].toString());
        if (!v.textColor.isValid())
            return false;
    }
    if (o.contains("textOutline")) {
        if (!o["textOutline"].isBool())
            return false;
        v.textOutline = o["textOutline"].toBool();
    }
    if (o.contains("outlineColor")) {
        if (!o["outlineColor"].isString())
            return false;
        v.outlineColor = QColor(o["outlineColor"].toString());
        if (!v.outlineColor.isValid())
            return false;
    }
    if (o.contains("opacity")) {
        if (!o["opacity"].isDouble())
            return false;
        v.opacity = o["opacity"].toDouble();
    }
    if (o.contains("scalePercent")) {
        if (!o["scalePercent"].isDouble())
            return false;
        v.scalePercent = o["scalePercent"].toDouble();
    }
    if (o.contains("position")) {
        if (!o["position"].isDouble())
            return false;
        if (o["position"].toDouble() != o["position"].toInt())
            return false;
        v.position = static_cast<decltype(v.position)>(o["position"].toInt());
    }
    if (o.contains("marginX")) {
        if (!o["marginX"].isDouble())
            return false;
        if (o["marginX"].toDouble() != o["marginX"].toInt())
            return false;
        v.marginX = o["marginX"].toInt();
    }
    if (o.contains("marginY")) {
        if (!o["marginY"].isDouble())
            return false;
        if (o["marginY"].toDouble() != o["marginY"].toInt())
            return false;
        v.marginY = o["marginY"].toInt();
    }
    if (o.contains("rotationDegrees")) {
        if (!o["rotationDegrees"].isDouble())
            return false;
        v.rotationDegrees = o["rotationDegrees"].toDouble();
    }
    if (o.contains("watermarkImagePng")) {
        if (!o["watermarkImagePng"].isString())
            return false;
        auto bytes = QByteArray::fromBase64(o["watermarkImagePng"].toString().toLatin1());
        if (!bytes.isEmpty() && !v.watermarkImage.loadFromData(bytes, "PNG"))
            return false;
    }
    return true;
}
QJsonObject encodeMassRenameOptions(const MassRenameOptions &v) {
    QJsonObject o;
    o["pattern"] = v.pattern;
    o["prefix"] = v.prefix;
    o["suffix"] = v.suffix;
    o["findText"] = v.findText;
    o["replaceText"] = v.replaceText;
    o["startNumber"] = v.startNumber;
    o["numberStep"] = v.numberStep;
    o["zeroPadding"] = v.zeroPadding;
    o["forceExtension"] = v.forceExtension;
    return o;
}
bool decodeMassRenameOptions(const QJsonObject &o, MassRenameOptions &v) {
    if (o.contains("pattern")) {
        if (!o["pattern"].isString())
            return false;
        v.pattern = o["pattern"].toString();
    }
    if (o.contains("prefix")) {
        if (!o["prefix"].isString())
            return false;
        v.prefix = o["prefix"].toString();
    }
    if (o.contains("suffix")) {
        if (!o["suffix"].isString())
            return false;
        v.suffix = o["suffix"].toString();
    }
    if (o.contains("findText")) {
        if (!o["findText"].isString())
            return false;
        v.findText = o["findText"].toString();
    }
    if (o.contains("replaceText")) {
        if (!o["replaceText"].isString())
            return false;
        v.replaceText = o["replaceText"].toString();
    }
    if (o.contains("startNumber")) {
        if (!o["startNumber"].isDouble())
            return false;
        if (o["startNumber"].toDouble() != o["startNumber"].toInt())
            return false;
        v.startNumber = o["startNumber"].toInt();
    }
    if (o.contains("numberStep")) {
        if (!o["numberStep"].isDouble())
            return false;
        if (o["numberStep"].toDouble() != o["numberStep"].toInt())
            return false;
        v.numberStep = o["numberStep"].toInt();
    }
    if (o.contains("zeroPadding")) {
        if (!o["zeroPadding"].isDouble())
            return false;
        if (o["zeroPadding"].toDouble() != o["zeroPadding"].toInt())
            return false;
        v.zeroPadding = o["zeroPadding"].toInt();
    }
    if (o.contains("forceExtension")) {
        if (!o["forceExtension"].isString())
            return false;
        v.forceExtension = o["forceExtension"].toString();
    }
    return true;
}
} // namespace
QJsonObject BatchProcessor::presetToJson(const BatchPreset &p) {
    auto o = encodeBatchPreset(p);
    o["schemaVersion"] = 1;
    o["resizeOptions"] = encodeResizeOptions(p.resizeOptions);
    o["colorOptions"] = encodeColorAdjustOptions(p.colorOptions);
    o["watermarkOptions"] = encodeWatermarkOptions(p.watermarkOptions);
    o["renameOptions"] = encodeMassRenameOptions(p.renameOptions);
    return o;
}
Result<BatchPreset> BatchProcessor::presetFromJson(const QJsonObject &o) {
    BatchPreset p;
    auto fail = []() {
        return Result<BatchPreset>::failure(ErrorCode::SchemaValidationError,
                                            "Invalid batch preset JSON");
    };
    if (o.contains("schemaVersion") && o["schemaVersion"] != QJsonValue(1))
        return fail();
    for (const char *key : {"resizeOptions", "colorOptions", "watermarkOptions", "renameOptions"})
        if (o.contains(key) && !o[key].isObject())
            return fail();
    if (!decodeBatchPreset(o, p) ||
        !decodeResizeOptions(o["resizeOptions"].toObject(), p.resizeOptions) ||
        !decodeColorAdjustOptions(o["colorOptions"].toObject(), p.colorOptions) ||
        !decodeWatermarkOptions(o["watermarkOptions"].toObject(), p.watermarkOptions) ||
        !decodeMassRenameOptions(o["renameOptions"].toObject(), p.renameOptions))
        return fail();
    auto &r = p.resizeOptions;
    auto &c = p.colorOptions;
    auto &w = p.watermarkOptions;
    if (p.outputFormat.isEmpty() || p.quality < 0 || p.quality > 100 || int(r.mode) < 0 ||
        int(r.mode) > 2 || int(r.interpolation) < 0 || int(r.interpolation) > 3 ||
        int(w.position) < 0 || int(w.position) > 9 || r.percentage <= 0 || w.fontPointSize <= 0 ||
        w.opacity < 0 || w.opacity > 1 || w.scalePercent <= 0 || w.scalePercent > 1000 ||
        w.marginX < 0 || w.marginY < 0 || p.renameOptions.zeroPadding < 0 ||
        p.renameOptions.zeroPadding > 32)
        return fail();
    QImage pixel(1, 1, QImage::Format_ARGB32);
    pixel.fill(Qt::black);
    if (!ColorAdjustOp::apply(pixel, c).isOk())
        return fail();
    if (p.enableResize && !ResizeOp::computeTargetSize(QSize(100, 100), r).isOk())
        return fail();
    return Result<BatchPreset>::success(p);
}
QJsonObject presetToJson(const BatchPreset &p) { return BatchProcessor::presetToJson(p); }
Result<BatchPreset> presetFromJson(const QJsonObject &o) {
    return BatchProcessor::presetFromJson(o);
}
} // namespace photo26::core
