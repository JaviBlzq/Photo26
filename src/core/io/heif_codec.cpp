#include "heif_codec.hpp"

#include <QColorSpace>
#include <QFile>
#include <QFileInfo>
#include <QScopeGuard>
#include <QtEndian>
#include <algorithm>
#include <cstring>
#include <mutex>
#include <vector>

#ifdef PHOTO26_HAS_LIBHEIF
#include <libheif/heif.h>
#endif

namespace photo26::core {

namespace {

#ifdef PHOTO26_HAS_LIBHEIF
void ensureHeifInit() {
    static std::once_flag flag;
    std::call_once(flag, [] {
        heif_init(nullptr);
    });
}

bool withinLimits(int w, int h, const ImageLimits& l) {
    return w > 0 && h > 0 && w <= l.maxWidth && h <= l.maxHeight &&
           quint64(w) * quint64(h) <= l.maxPixels;
}

void extractBlocksFromHandle(const heif_image_handle* handle, HeifMetadataBlocks& out) {
    out.dimensions = QSize(
        heif_image_handle_get_width(handle),
        heif_image_handle_get_height(handle));
    out.hasAlpha = heif_image_handle_has_alpha_channel(handle) != 0;

    // ICC profile (rICC / prof)
    const auto profileType = heif_image_handle_get_color_profile_type(handle);
    if (profileType == heif_color_profile_type_rICC || profileType == heif_color_profile_type_prof) {
        const size_t iccSize = heif_image_handle_get_raw_color_profile_size(handle);
        if (iccSize > 0 && iccSize <= 16u * 1024u * 1024u) {
            QByteArray icc(int(iccSize), Qt::Uninitialized);
            const heif_error err = heif_image_handle_get_raw_color_profile(handle, icc.data());
            if (err.code == heif_error_Ok) {
                out.iccBytes = std::move(icc);
            }
        }
    }

    // EXIF block (first 4 bytes in HEIF EXIF item are the big-endian TIFF header offset)
    heif_item_id exifId = 0;
    if (heif_image_handle_get_list_of_metadata_block_IDs(handle, "Exif", &exifId, 1) > 0) {
        const size_t exifSize = heif_image_handle_get_metadata_size(handle, exifId);
        if (exifSize > 4 && exifSize <= 16u * 1024u * 1024u) {
            QByteArray raw(int(exifSize), Qt::Uninitialized);
            const heif_error err = heif_image_handle_get_metadata(handle, exifId, raw.data());
            if (err.code == heif_error_Ok) {
                const quint32 tiffOffset = qFromBigEndian<quint32>(raw.constData());
                const qsizetype payloadStart = 4 + qsizetype(tiffOffset);
                if (payloadStart >= 4 && payloadStart < raw.size()) {
                    out.exifBytes = raw.mid(payloadStart);
                }
            }
        }
    }

    // XMP block (mime item with content_type application/rdf+xml)
    const int mimeCount = heif_image_handle_get_number_of_metadata_blocks(handle, "mime");
    if (mimeCount > 0) {
        std::vector<heif_item_id> ids(size_t(std::min(mimeCount, 32)));
        const int got = heif_image_handle_get_list_of_metadata_block_IDs(
            handle, "mime", ids.data(), int(ids.size()));
        for (int i = 0; i < got; ++i) {
            const char* ctype = heif_image_handle_get_metadata_content_type(handle, ids[size_t(i)]);
            if (ctype && QString::fromLatin1(ctype).contains(QStringLiteral("rdf+xml"), Qt::CaseInsensitive)) {
                const size_t xmpSize = heif_image_handle_get_metadata_size(handle, ids[size_t(i)]);
                if (xmpSize > 0 && xmpSize <= 16u * 1024u * 1024u) {
                    QByteArray xmp(int(xmpSize), Qt::Uninitialized);
                    if (heif_image_handle_get_metadata(handle, ids[size_t(i)], xmp.data()).code == heif_error_Ok) {
                        out.xmpBytes = std::move(xmp);
                        break;
                    }
                }
            }
        }
    }
}

heif_error heifWriteCallback(heif_context* /*ctx*/, const void* data, size_t size, void* userdata) {
    auto* buffer = static_cast<QByteArray*>(userdata);
    if (data && size > 0) {
        buffer->append(static_cast<const char*>(data), qsizetype(size));
    }
    return heif_error{heif_error_Ok, heif_suberror_Unspecified, ""};
}
#endif

} // namespace

bool HeifCodec::isAvailable() noexcept {
#ifdef PHOTO26_HAS_LIBHEIF
    ensureHeifInit();
    return true;
#else
    return false;
#endif
}

bool HeifCodec::canReadHeic() noexcept {
#ifdef PHOTO26_HAS_LIBHEIF
    ensureHeifInit();
    return heif_have_decoder_for_format(heif_compression_HEVC) != 0;
#else
    return false;
#endif
}

bool HeifCodec::canWriteHeic() noexcept {
#ifdef PHOTO26_HAS_LIBHEIF
    ensureHeifInit();
    return heif_have_encoder_for_format(heif_compression_HEVC) != 0;
#else
    return false;
#endif
}

bool HeifCodec::canReadAvif() noexcept {
#ifdef PHOTO26_HAS_LIBHEIF
    ensureHeifInit();
    return heif_have_decoder_for_format(heif_compression_AV1) != 0;
#else
    return false;
#endif
}

bool HeifCodec::canWriteAvif() noexcept {
#ifdef PHOTO26_HAS_LIBHEIF
    ensureHeifInit();
    return heif_have_encoder_for_format(heif_compression_AV1) != 0;
#else
    return false;
#endif
}

QString HeifCodec::libraryVersion() {
#ifdef PHOTO26_HAS_LIBHEIF
    ensureHeifInit();
    return QString::fromLatin1(heif_get_version());
#else
    return {};
#endif
}

Result<QSize> HeifCodec::probeSize(const QString& filePath) {
    using R = Result<QSize>;
#ifndef PHOTO26_HAS_LIBHEIF
    Q_UNUSED(filePath);
    return R::failure(ErrorCode::UnsupportedFormat, "libheif no disponible en esta compilación");
#else
    ensureHeifInit();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return R::failure(ErrorCode::FileReadError, "No se pudo abrir el archivo HEIF/AVIF", file.errorString());
    }
    const QByteArray bytes = file.readAll();
    auto meta = extractMetadataBlocks(bytes);
    if (!meta.isOk()) {
        return R::failure(meta.error.code, meta.error.message, meta.error.detail);
    }
    return R::success(meta.value->dimensions);
#endif
}

Result<HeifMetadataBlocks> HeifCodec::extractMetadataBlocks(const QByteArray& bytes) {
    using R = Result<HeifMetadataBlocks>;
#ifndef PHOTO26_HAS_LIBHEIF
    Q_UNUSED(bytes);
    return R::failure(ErrorCode::UnsupportedFormat, "libheif no disponible en esta compilación");
#else
    ensureHeifInit();
    if (bytes.isEmpty()) {
        return R::failure(ErrorCode::CorruptedInput, "Archivo HEIF/AVIF vacío");
    }
    heif_context* ctx = heif_context_alloc();
    if (!ctx) {
        return R::failure(ErrorCode::InternalError, "No se pudo reservar contexto libheif");
    }
    auto freeCtx = qScopeGuard([ctx] { heif_context_free(ctx); });

    heif_error err = heif_context_read_from_memory_without_copy(ctx, bytes.constData(), size_t(bytes.size()), nullptr);
    if (err.code != heif_error_Ok) {
        return R::failure(ErrorCode::CorruptedInput, "Contenedor HEIC/HEIF/AVIF inválido", QString::fromUtf8(err.message));
    }

    heif_image_handle* handle = nullptr;
    err = heif_context_get_primary_image_handle(ctx, &handle);
    if (err.code != heif_error_Ok || !handle) {
        return R::failure(ErrorCode::CorruptedInput, "No se encontró imagen principal HEIF/AVIF", QString::fromUtf8(err.message));
    }
    auto freeHandle = qScopeGuard([handle] { heif_image_handle_release(handle); });

    HeifMetadataBlocks blocks;
    extractBlocksFromHandle(handle, blocks);
    return R::success(std::move(blocks));
#endif
}

Result<HeifDecodedResult> HeifCodec::decodeFromBytes(const QByteArray& bytes, const ImageLimits& limits) {
    using R = Result<HeifDecodedResult>;
#ifndef PHOTO26_HAS_LIBHEIF
    Q_UNUSED(bytes);
    Q_UNUSED(limits);
    return R::failure(ErrorCode::UnsupportedFormat, "libheif no disponible en esta compilación");
#else
    ensureHeifInit();
    if (bytes.isEmpty()) {
        return R::failure(ErrorCode::CorruptedInput, "Archivo HEIC/HEIF/AVIF vacío");
    }

    heif_context* ctx = heif_context_alloc();
    if (!ctx) {
        return R::failure(ErrorCode::InternalError, "No se pudo reservar contexto libheif");
    }
    auto freeCtx = qScopeGuard([ctx] { heif_context_free(ctx); });

    heif_error err = heif_context_read_from_memory_without_copy(ctx, bytes.constData(), size_t(bytes.size()), nullptr);
    if (err.code != heif_error_Ok) {
        return R::failure(ErrorCode::CorruptedInput, "No se pudo leer el contenedor HEIC/HEIF/AVIF", QString::fromUtf8(err.message));
    }

    heif_image_handle* handle = nullptr;
    err = heif_context_get_primary_image_handle(ctx, &handle);
    if (err.code != heif_error_Ok || !handle) {
        return R::failure(ErrorCode::CorruptedInput, "No se pudo obtener la imagen principal HEIF/AVIF", QString::fromUtf8(err.message));
    }
    auto freeHandle = qScopeGuard([handle] { heif_image_handle_release(handle); });

    const int width = heif_image_handle_get_width(handle);
    const int height = heif_image_handle_get_height(handle);
    if (!withinLimits(width, height, limits)) {
        return R::failure(ErrorCode::ImageDimensionsTooLarge, "Dimensiones HEIC/HEIF/AVIF fuera de los límites");
    }

    HeifMetadataBlocks blocks;
    extractBlocksFromHandle(handle, blocks);

    heif_decoding_options* decOpts = heif_decoding_options_alloc();
    auto freeDecOpts = qScopeGuard([decOpts] { if (decOpts) heif_decoding_options_free(decOpts); });
    if (decOpts) {
        // Apply ISOBMFF irot/imir/clap transforms directly; EXIF orientation is handled consistently by ImageIo.
        decOpts->ignore_transformations = 0;
    }

    const bool hasAlpha = blocks.hasAlpha;
    heif_image* heifImg = nullptr;
    err = heif_decode_image(
        handle,
        &heifImg,
        heif_colorspace_RGB,
        hasAlpha ? heif_chroma_interleaved_RGBA : heif_chroma_interleaved_RGB,
        decOpts);
    if (err.code != heif_error_Ok || !heifImg) {
        return R::failure(ErrorCode::CorruptedInput, "No se pudo decodificar la imagen HEIC/HEIF/AVIF", QString::fromUtf8(err.message));
    }
    auto freeImg = qScopeGuard([heifImg] { heif_image_release(heifImg); });

    const int decodedW = heif_image_get_width(heifImg, heif_channel_interleaved);
    const int decodedH = heif_image_get_height(heifImg, heif_channel_interleaved);
    if (!withinLimits(decodedW, decodedH, limits)) {
        return R::failure(ErrorCode::ImageDimensionsTooLarge, "Dimensiones decodificadas HEIC/HEIF/AVIF fuera de los límites");
    }

    int stride = 0;
    const uint8_t* plane = heif_image_get_plane_readonly(heifImg, heif_channel_interleaved, &stride);
    if (!plane || stride <= 0) {
        return R::failure(ErrorCode::CorruptedInput, "Plano de píxeles HEIC/HEIF/AVIF inválido");
    }

    QImage out(decodedW, decodedH, hasAlpha ? QImage::Format_RGBA8888 : QImage::Format_RGB888);
    if (out.isNull()) {
        return R::failure(ErrorCode::InternalError, "Sin memoria para decodificar imagen HEIC/HEIF/AVIF");
    }
    const int rowBytes = decodedW * (hasAlpha ? 4 : 3);
    for (int y = 0; y < decodedH; ++y) {
        std::memcpy(out.scanLine(y), plane + size_t(y) * size_t(stride), size_t(rowBytes));
    }

    if (!blocks.iccBytes.isEmpty()) {
        const QColorSpace cs = QColorSpace::fromIccProfile(blocks.iccBytes);
        if (cs.isValid()) {
            out.setColorSpace(cs);
        }
    }

    HeifDecodedResult result;
    result.image = hasAlpha ? out.convertToFormat(QImage::Format_ARGB32) : out.convertToFormat(QImage::Format_RGB32);
    result.image.setColorSpace(out.colorSpace());
    result.exifBytes = std::move(blocks.exifBytes);
    result.xmpBytes = std::move(blocks.xmpBytes);
    result.iccBytes = std::move(blocks.iccBytes);
    return R::success(std::move(result));
#endif
}

Result<HeifDecodedResult> HeifCodec::decodeFromFile(const QString& filePath, const ImageLimits& limits) {
    using R = Result<HeifDecodedResult>;
    QFileInfo fi(filePath);
    if (!fi.exists() || !fi.isFile()) {
        return R::failure(ErrorCode::FileNotFound, "Archivo HEIC/HEIF/AVIF inexistente", filePath);
    }
    if (quint64(fi.size()) > limits.maxFileBytes) {
        return R::failure(ErrorCode::ImageDimensionsTooLarge, "El archivo supera el límite de tamaño");
    }
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return R::failure(ErrorCode::FileReadError, "No se pudo leer el archivo HEIC/HEIF/AVIF", file.errorString());
    }
    return decodeFromBytes(file.readAll(), limits);
}

Result<QByteArray> HeifCodec::encodeToBytes(
    const QImage& image,
    const QString& formatId,
    int quality,
    bool lossless,
    const QByteArray& exifBytes,
    const QByteArray& xmpBytes,
    const QByteArray& iccBytes) {
    using R = Result<QByteArray>;
#ifndef PHOTO26_HAS_LIBHEIF
    Q_UNUSED(image); Q_UNUSED(formatId); Q_UNUSED(quality); Q_UNUSED(lossless);
    Q_UNUSED(exifBytes); Q_UNUSED(xmpBytes); Q_UNUSED(iccBytes);
    return R::failure(ErrorCode::UnsupportedFormat, "libheif no disponible en esta compilación");
#else
    ensureHeifInit();
    if (image.isNull() || image.width() <= 0 || image.height() <= 0) {
        return R::failure(ErrorCode::InvalidParameter, "Imagen vacía para codificar HEIC/HEIF/AVIF");
    }

    const QString fmt = formatId.trimmed().toLower();
    const bool isAvif = (fmt == QStringLiteral("avif") || fmt == QStringLiteral("avis"));
    const heif_compression_format compression = isAvif ? heif_compression_AV1 : heif_compression_HEVC;

    heif_context* ctx = heif_context_alloc();
    if (!ctx) {
        return R::failure(ErrorCode::InternalError, "No se pudo crear contexto libheif");
    }
    auto freeCtx = qScopeGuard([ctx] { heif_context_free(ctx); });

    heif_encoder* encoder = nullptr;
    heif_error err = heif_context_get_encoder_for_format(ctx, compression, &encoder);
    if (err.code != heif_error_Ok || !encoder) {
        return R::failure(ErrorCode::UnsupportedFormat, "No hay codificador HEVC/AV1 disponible en libheif", QString::fromUtf8(err.message));
    }
    auto freeEncoder = qScopeGuard([encoder] { heif_encoder_release(encoder); });

    const int q = std::clamp(quality < 0 ? 85 : quality, 1, 100);
    if (lossless) {
        heif_encoder_set_lossless(encoder, 1);
    } else {
        heif_encoder_set_lossless(encoder, 0);
        heif_encoder_set_lossy_quality(encoder, q);
    }

    // Use a fast practical preset for desktop responsiveness when supported by the encoder.
    if (!isAvif) {
        heif_encoder_set_parameter_string(encoder, "preset", "fast");
    } else {
        heif_encoder_set_parameter_integer(encoder, "speed", 6);
    }

    const bool hasAlpha = image.hasAlphaChannel();
    const QImage converted = image.convertToFormat(hasAlpha ? QImage::Format_RGBA8888 : QImage::Format_RGB888);
    if (converted.isNull()) {
        return R::failure(ErrorCode::InternalError, "No se pudo convertir el formato de píxeles para HEIF/AVIF");
    }

    heif_image* heifImg = nullptr;
    err = heif_image_create(
        converted.width(),
        converted.height(),
        heif_colorspace_RGB,
        hasAlpha ? heif_chroma_interleaved_RGBA : heif_chroma_interleaved_RGB,
        &heifImg);
    if (err.code != heif_error_Ok || !heifImg) {
        return R::failure(ErrorCode::InternalError, "No se pudo reservar imagen libheif", QString::fromUtf8(err.message));
    }
    auto freeImg = qScopeGuard([heifImg] { heif_image_release(heifImg); });

    err = heif_image_add_plane(heifImg, heif_channel_interleaved, converted.width(), converted.height(), 8);
    if (err.code != heif_error_Ok) {
        return R::failure(ErrorCode::InternalError, "No se pudo crear plano de píxeles libheif", QString::fromUtf8(err.message));
    }

    int dstStride = 0;
    uint8_t* dstPlane = heif_image_get_plane(heifImg, heif_channel_interleaved, &dstStride);
    if (!dstPlane || dstStride <= 0) {
        return R::failure(ErrorCode::InternalError, "Plano de destino libheif inválido");
    }
    const int rowBytes = converted.width() * (hasAlpha ? 4 : 3);
    for (int y = 0; y < converted.height(); ++y) {
        std::memcpy(dstPlane + size_t(y) * size_t(dstStride), converted.constScanLine(y), size_t(rowBytes));
    }

    QByteArray profileBytes = iccBytes;
    if (profileBytes.isEmpty() && image.colorSpace().isValid()) {
        profileBytes = image.colorSpace().iccProfile();
    }
    if (!profileBytes.isEmpty()) {
        heif_image_set_raw_color_profile(heifImg, "prof", profileBytes.constData(), size_t(profileBytes.size()));
    }

    heif_encoding_options* encOpts = heif_encoding_options_alloc();
    auto freeEncOpts = qScopeGuard([encOpts] { if (encOpts) heif_encoding_options_free(encOpts); });
    if (encOpts) {
        encOpts->save_alpha_channel = hasAlpha ? 1 : 0;
    }

    heif_image_handle* outHandle = nullptr;
    err = heif_context_encode_image(ctx, heifImg, encoder, encOpts, &outHandle);
    if (err.code != heif_error_Ok || !outHandle) {
        return R::failure(ErrorCode::FileWriteError, "Error codificando imagen HEIC/HEIF/AVIF", QString::fromUtf8(err.message));
    }
    auto freeOutHandle = qScopeGuard([outHandle] { heif_image_handle_release(outHandle); });

    if (!exifBytes.isEmpty()) {
        // libheif expects raw TIFF header bytes; heif_context_add_exif_metadata prepends the 4-byte offset automatically.
        QByteArray rawExif = exifBytes;
        if (rawExif.startsWith(QByteArray("Exif\0\0", 6))) {
            rawExif.remove(0, 6);
        }
        heif_context_add_exif_metadata(ctx, outHandle, rawExif.constData(), int(rawExif.size()));
    }
    if (!xmpBytes.isEmpty()) {
        heif_context_add_XMP_metadata(ctx, outHandle, xmpBytes.constData(), int(xmpBytes.size()));
    }

    QByteArray encoded;
    heif_writer writer{};
    writer.writer_api_version = 1;
    writer.write = &heifWriteCallback;
    err = heif_context_write(ctx, &writer, &encoded);
    if (err.code != heif_error_Ok || encoded.isEmpty()) {
        return R::failure(ErrorCode::FileWriteError, "Error escribiendo contenedor HEIC/HEIF/AVIF", QString::fromUtf8(err.message));
    }

    return R::success(std::move(encoded));
#endif
}

} // namespace photo26::core
