#include "combine_op.hpp"
#include <QtGui/QPainter>
#include <algorithm>
#include <cmath>
namespace photo26::core {
Result<QImage> CombineOp::combine(const std::vector<QImage> &images, const CombineOptions &o) {
    if (images.empty() || images.size() > 100000 || o.spacingPx < 0 || o.marginPx < 0 ||
        o.targetCellWidth < 0 || o.targetCellHeight < 0 || o.gridColumns <= 0 ||
        o.spacingPx > 32768 || o.marginPx > 32768 || o.targetCellWidth > 32768 ||
        o.targetCellHeight > 32768)
        return Result<QImage>::failure(ErrorCode::InvalidParameter, "Invalid combination options");
    int cw = o.targetCellWidth, ch = o.targetCellHeight;
    for (auto &i : images) {
        if (i.isNull())
            return Result<QImage>::failure(ErrorCode::InvalidParameter, "Empty image");
        if (!o.targetCellWidth)
            cw = std::max(cw, i.width());
        if (!o.targetCellHeight)
            ch = std::max(ch, i.height());
    }
    int n = int(images.size()),
        cols = o.layout == CombineLayout::Horizontal ? n
               : o.layout == CombineLayout::Vertical ? 1
                                                     : std::min(n, o.gridColumns),
        rows = (n + cols - 1) / cols;
    // Mosaic uses variable-width proportional tiles in rows of common height.
    std::vector<QRect> cells;
    long long width = 0, height = 0;
    if (o.layout == CombineLayout::Mosaic) {
        int y = o.marginPx;
        for (int start = 0; start < n; start += cols) {
            int x = o.marginPx;
            for (int i = start; i < std::min(n, start + cols); ++i) {
                double wd = double(images[i].width()) * ch / images[i].height();
                if (wd > 32768)
                    return Result<QImage>::failure(ErrorCode::ImageDimensionsTooLarge,
                                                   "Mosaic tile too wide");
                int w = std::max(1, int(std::lround(wd)));
                if (1LL * x + w + o.marginPx > 32768 || 1LL * y + ch + o.marginPx > 32768)
                    return Result<QImage>::failure(ErrorCode::ImageDimensionsTooLarge,
                                                   "Mosaic exceeds canvas limits");
                cells.emplace_back(x, y, w, ch);
                x += w + o.spacingPx;
            }
            width = std::max(width, static_cast<long long>(x - o.spacingPx + o.marginPx));
            y += ch + o.spacingPx;
        }
        height = static_cast<long long>(o.marginPx) * 2 + static_cast<long long>(rows) * ch +
                 static_cast<long long>(rows - 1) * o.spacingPx;
    } else {
        width = 2LL * o.marginPx + 1LL * cols * cw + 1LL * (cols - 1) * o.spacingPx;
        height = 2LL * o.marginPx + 1LL * rows * ch + 1LL * (rows - 1) * o.spacingPx;
        if (width <= 32768 && height <= 32768)
            for (int i = 0; i < n; ++i)
                cells.emplace_back(o.marginPx + (i % cols) * (cw + o.spacingPx),
                                   o.marginPx + (i / cols) * (ch + o.spacingPx), cw, ch);
    }
    if (width <= 0 || height <= 0 || width > 32768 || height > 32768 || width * height > 134217728)
        return Result<QImage>::failure(ErrorCode::ImageDimensionsTooLarge, "Canvas exceeds limits");
    QImage out(int(width), int(height), QImage::Format_ARGB32_Premultiplied);
    if (out.isNull())
        return Result<QImage>::failure(ErrorCode::InternalError, "Allocation failed");
    out.fill(o.transparentBackground ? QColor(Qt::transparent) : o.backgroundColor);
    QPainter p(&out);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    for (int i = 0; i < n; ++i) {
        QRect cell = cells[i];
        QSize size = images[i].size();
        if (o.scalePolicy != CombineScalePolicy::KeepOriginalSize)
            size.scale(cell.size(), o.scalePolicy == CombineScalePolicy::FillCellCrop
                                        ? Qt::KeepAspectRatioByExpanding
                                        : Qt::KeepAspectRatio);
        auto offset = [&](int delta) {
            return o.alignment == CombineAlignment::Start ? 0
                   : o.alignment == CombineAlignment::End ? delta
                                                          : delta / 2;
        };
        QRect dest(cell.x() + offset(cell.width() - size.width()),
                   cell.y() + offset(cell.height() - size.height()), size.width(), size.height());
        p.save();
        p.setClipRect(cell);
        p.drawImage(dest, images[i]);
        p.restore();
    }
    p.end();
    return Result<QImage>::success(out);
}
} // namespace photo26::core
