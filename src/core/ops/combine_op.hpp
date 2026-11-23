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
#ifndef PHOTO26_OP_COMBINE_OP
#define PHOTO26_OP_COMBINE_OP
enum class CombineLayout { Horizontal, Vertical, Grid, Mosaic };
enum class CombineAlignment { Start, Center, End };
enum class CombineScalePolicy { KeepOriginalSize, FitProportionally, FillCellCrop };
struct CombineOptions {
    CombineLayout layout{CombineLayout::Horizontal};
    int gridColumns{2};
    int spacingPx{8};
    int marginPx{12};
    QColor backgroundColor{Qt::white};
    bool transparentBackground{false};
    CombineAlignment alignment{CombineAlignment::Center};
    CombineScalePolicy scalePolicy{CombineScalePolicy::FitProportionally};
    int targetCellWidth{0};
    int targetCellHeight{0};
};
class CombineOp {
  public:
    static Result<QImage> combine(const std::vector<QImage> &, const CombineOptions & = {});
};
#endif
} // namespace photo26::core
