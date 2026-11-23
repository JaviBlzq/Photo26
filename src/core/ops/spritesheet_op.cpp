#include "spritesheet_op.hpp"
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QBuffer>
#include <QtCore/QFileInfo>
#include <QtGui/QPainter>
#include <QtGui/QImageWriter>
#include <algorithm>
#include <cmath>
namespace photo26::core {
Result<SpritesheetResult> SpritesheetOp::generate(const std::vector<SpriteInput> &sprites,
                                                  const SpritesheetOptions &o) {
    if (sprites.empty() || sprites.size() > 100000 || o.columns < 0 || o.rows < 0 ||
        o.spacingPx < 0 || o.paddingPx < 0 || o.forcedCellWidth < 0 || o.forcedCellHeight < 0)
        return Result<SpritesheetResult>::failure(ErrorCode::InvalidParameter,
                                                  "Invalid spritesheet options");
    int cw = o.forcedCellWidth, ch = o.forcedCellHeight;
    for (auto &s : sprites) {
        if (s.image.isNull())
            return Result<SpritesheetResult>::failure(ErrorCode::InvalidParameter, "Empty sprite");
        if (!o.forcedCellWidth)
            cw = std::max(cw, s.image.width());
        if (!o.forcedCellHeight)
            ch = std::max(ch, s.image.height());
    }
    int n = int(sprites.size()),
        cols = o.columns ? o.columns
               : o.rows  ? (n + o.rows - 1) / o.rows
                         : int(std::ceil(std::sqrt(double(n)))),
        rows = o.rows ? o.rows : (n + cols - 1) / cols;
    if (1LL * rows * cols < n)
        return Result<SpritesheetResult>::failure(ErrorCode::InvalidParameter,
                                                  "Grid cannot contain all sprites");
    long long w = 2LL * o.paddingPx + 1LL * cols * cw + 1LL * (cols - 1) * o.spacingPx,
              h = 2LL * o.paddingPx + 1LL * rows * ch + 1LL * (rows - 1) * o.spacingPx;
    if (w > 32768 || h > 32768 || w * h > 134217728)
        return Result<SpritesheetResult>::failure(ErrorCode::ImageDimensionsTooLarge,
                                                  "Atlas too large");
    SpritesheetResult out;
    out.atlasImage = QImage(int(w), int(h), QImage::Format_ARGB32_Premultiplied);
    if (out.atlasImage.isNull())
        return Result<SpritesheetResult>::failure(ErrorCode::InternalError, "Allocation failed");
    out.atlasImage.fill(o.backgroundColor);
    QPainter p(&out.atlasImage);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    QJsonArray frames;
    for (int i = 0; i < n; ++i) {
        QSize size = sprites[i].image.size();
        if (size.width() > cw || size.height() > ch)
            size.scale(cw, ch, Qt::KeepAspectRatio);
        int x = o.paddingPx + (i % cols) * (cw + o.spacingPx) + (cw - size.width()) / 2,
            y = o.paddingPx + (i / cols) * (ch + o.spacingPx) + (ch - size.height()) / 2;
        p.drawImage(QRect(QPoint(x, y), size), sprites[i].image);
        out.frames.push_back({sprites[i].name, x, y, size.width(), size.height()});
        frames.append(QJsonObject{{"name", sprites[i].name},
                                  {"x", x},
                                  {"y", y},
                                  {"width", size.width()},
                                  {"height", size.height()}});
    }
    p.end();
    out.coordinatesJson =
        QJsonDocument(QJsonObject{{"width", int(w)}, {"height", int(h)}, {"frames", frames}})
            .toJson();
    return Result<SpritesheetResult>::success(std::move(out));
}
StatusResult SpritesheetOp::saveSpritesheetAndJson(const SpritesheetResult &r, const QString &ip,
                                                   const QString &jp, const QString &fmt,
                                                   bool overwrite) {
    if (r.atlasImage.isNull() || QFileInfo(ip).absoluteFilePath().compare(
                                     QFileInfo(jp).absoluteFilePath(), Qt::CaseInsensitive) == 0)
        return StatusResult::failure(ErrorCode::InvalidParameter,
                                     "Invalid atlas or matching output paths");
    if (!overwrite && (QFileInfo::exists(ip) || QFileInfo::exists(jp)))
        return StatusResult::failure(ErrorCode::ConflictDetected, "Output exists");
    QByteArray bytes;
    QBuffer b(&bytes);
    b.open(QIODevice::WriteOnly);
    QImageWriter wr(&b, fmt.toLatin1());
    if (!wr.write(r.atlasImage))
        return StatusResult::failure(ErrorCode::FileWriteError, wr.errorString());
    auto st = SafeFileWriter::writeBytes(ip, bytes, {overwrite, true});
    if (!st.isOk())
        return st;
    return SafeFileWriter::writeBytes(jp, r.coordinatesJson, {overwrite, true});
}
} // namespace photo26::core
