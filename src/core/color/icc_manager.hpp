#pragma once
#include "../common/result.hpp"
#include <QColorSpace>
namespace photo26::core {
enum class TargetColorSpace { KeepOriginal, SRgb, AdobeRgb, DisplayP3, ProPhotoRgb };
struct IccProfileInfo { bool hasProfile{false}; QString description; QString colorSpaceName; QByteArray rawIccBytes; int byteSize{0}; };
class IccManager {
public:
    static IccProfileInfo inspect(const QImage& image);
    static IccProfileInfo inspectFile(const QString& filePath);
    static Result<QImage> convertToColorSpace(const QImage& image, TargetColorSpace target);
    // Assignment only: this tags pixels; use convertToColorSpace to transform values.
    static void applyColorSpace(QImage& image, TargetColorSpace target);
    static QColorSpace resolveQtColorSpace(TargetColorSpace target);
};
}
