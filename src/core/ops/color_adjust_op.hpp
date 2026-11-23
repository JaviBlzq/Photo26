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
#ifndef PHOTO26_OP_COLOR_ADJUST_OP
#define PHOTO26_OP_COLOR_ADJUST_OP
struct ColorAdjustOptions {
    bool grayscale{false};
    bool blackAndWhite{false};
    int bwThreshold{128};
    bool bwDither{false};
    bool sepia{false};
    double sepiaIntensity{1.0};
    bool invert{false};
    double brightness{0.0};
    double contrast{0.0};
    double saturation{0.0};
    double gamma{1.0};
    double exposureEv{0.0};
};
class ColorAdjustOp {
  public:
    static Result<QImage> apply(const QImage &, const ColorAdjustOptions &);
};
#endif
} // namespace photo26::core
