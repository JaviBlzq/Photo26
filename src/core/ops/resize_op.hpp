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
#ifndef PHOTO26_OP_RESIZE_OP
#define PHOTO26_OP_RESIZE_OP
enum class ResizeMode { ByPixels, ByPercentage, FitMaxDimensions };
enum class InterpolationMethod { Nearest, Bilinear, Bicubic, Lanczos3 };
struct ResizeOptions {
    ResizeMode mode{ResizeMode::ByPixels};
    int targetWidth{0};
    int targetHeight{0};
    double percentage{100.0};
    int maxWidth{1920};
    int maxHeight{1080};
    bool keepAspectRatio{true};
    bool preventUpscaling{false};
    InterpolationMethod interpolation{InterpolationMethod::Lanczos3};
};
class ResizeOp {
  public:
    static Result<QSize> computeTargetSize(const QSize &, const ResizeOptions &);
    static Result<QImage> resize(const QImage &, const ResizeOptions &);
};
#endif
} // namespace photo26::core
