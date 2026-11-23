#pragma once

#include "../common/result.hpp"
#include "../common/types.hpp"

#include <QByteArray>
#include <QImage>
#include <QSize>
#include <QString>

namespace photo26::core {

struct HeifDecodedResult {
    QImage image;
    QByteArray exifBytes;
    QByteArray xmpBytes;
    QByteArray iccBytes;
};

struct HeifMetadataBlocks {
    QByteArray exifBytes;
    QByteArray xmpBytes;
    QByteArray iccBytes;
    QSize dimensions;
    bool hasAlpha{false};
};

class HeifCodec {
public:
    static bool isAvailable() noexcept;
    static bool canReadHeic() noexcept;
    static bool canWriteHeic() noexcept;
    static bool canReadAvif() noexcept;
    static bool canWriteAvif() noexcept;
    static QString libraryVersion();

    static Result<QSize> probeSize(const QString& filePath);
    static Result<HeifMetadataBlocks> extractMetadataBlocks(const QByteArray& bytes);

    static Result<HeifDecodedResult> decodeFromBytes(
        const QByteArray& bytes,
        const ImageLimits& limits = {});

    static Result<HeifDecodedResult> decodeFromFile(
        const QString& filePath,
        const ImageLimits& limits = {});

    static Result<QByteArray> encodeToBytes(
        const QImage& image,
        const QString& formatId,
        int quality = 85,
        bool lossless = false,
        const QByteArray& exifBytes = {},
        const QByteArray& xmpBytes = {},
        const QByteArray& iccBytes = {});
};

} // namespace photo26::core
