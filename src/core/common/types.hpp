#pragma once

#include <QColor>
#include <QImage>
#include <QString>
#include <QStringList>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace photo26::core {

enum class ErrorCode {
    None = 0,
    FileNotFound,
    FileReadError,
    FileWriteError,
    PermissionDenied,
    UnsupportedFormat,
    CorruptedInput,
    ImageDimensionsTooLarge,
    InvalidParameter,
    MetadataIncompatible,
    SchemaValidationError,
    ConflictDetected,
    Cancelled,
    InternalError
};

struct OperationError {
    ErrorCode code{ErrorCode::None};
    QString message;
    QString detail;

    [[nodiscard]] bool isOk() const noexcept { return code == ErrorCode::None; }
};

struct ImageLimits {
    int maxWidth{32768};
    int maxHeight{32768};
    std::uint64_t maxPixels{134217728ULL}; // 128 MP default safety limit
    std::uint64_t maxFileBytes{536870912ULL}; // 512 MiB default input file limit
};

struct FormatCapability {
    QString id;              // e.g. "jpg", "png", "webp", "bmp", "tiff", "gif", "ico", "svg", "pdf", "heic", "heif", "avif"
    QString displayName;     // e.g. "JPEG (*.jpg *.jpeg)"
    QStringList extensions;  // e.g. {"jpg", "jpeg"}
    bool canRead{false};
    bool canWrite{false};
    bool supportsAlpha{false};
    bool supportsQuality{false};
    bool supportsLossless{false};
    bool supportsAnimation{false};
    bool supportsMultiPage{false};
    bool supportsExif{false};
    bool supportsXmp{false};
    bool supportsIptc{false};
    bool supportsIcc{false};
    bool isVector{false};
    QString notes;
};

struct SaveOptions {
    QString format; // e.g. "png", "jpg", "webp", "bmp", "tiff", "ico", "gif", "pdf"
    int quality{90}; // 1..100 (-1 for default)
    bool lossless{false};
    bool preserveTransparency{true};
    QColor backgroundColor{Qt::white}; // used when flattening alpha for opaque formats
    bool preserveMetadata{true};
    bool preserveIccProfile{true};
    int svgRasterWidth{0};  // 0 = intrinsic width
    int svgRasterHeight{0}; // 0 = intrinsic height
};

} // namespace photo26::core
