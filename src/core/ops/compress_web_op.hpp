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
#ifndef PHOTO26_OP_COMPRESS_WEB_OP
#define PHOTO26_OP_COMPRESS_WEB_OP
struct CompressionOptions {
    QString format{"jpg"};
    int quality{80};
    bool lossless{false};
    std::uint64_t targetMaxBytes{0};
    int maxWidth{0};
    int maxHeight{0};
    QColor backgroundColor{Qt::white};
};
struct CompressionMetrics {
    QByteArray encodedBytes;
    std::uint64_t originalBytes{0};
    std::uint64_t compressedBytes{0};
    int chosenQuality{80};
    double savingsPercent{0.0};
    QSize outputDimensions;
};
struct WebVariantSpec {
    int width{0};
    QString suffix;
};
struct WebOptimizerOptions {
    std::vector<int> widths{320, 640, 960, 1280, 1920};
    QString format{"webp"};
    int quality{82};
    bool lossless{false};
    bool stripMetadata{true};
    bool keepAspectRatio{true};
    bool preventUpscaling{true};
    QString namingPattern{"{name}-{width}w.{ext}"};
};
struct WebVariantResult {
    int targetWidth{0};
    QSize actualSize;
    QString fileName;
    QByteArray encodedBytes;
    std::uint64_t byteSize{0};
};
class CompressWebOp {
  public:
    static Result<CompressionMetrics> compressInMemory(const QImage &, const CompressionOptions &,
                                                       std::uint64_t = 0);
    static Result<std::vector<WebVariantResult>>
    generateWebVariants(const QImage &, const QString &, const WebOptimizerOptions & = {});
    static Result<QStringList> saveWebVariantsToFolder(const QImage &, const QString &,
                                                       const QString &,
                                                       const WebOptimizerOptions & = {},
                                                       bool = true);
};
#endif
} // namespace photo26::core
