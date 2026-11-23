#pragma once
#include "../common/result.hpp"
namespace photo26::core {
struct GifFrame { QImage image; int delayMs{100}; };
struct GifEncodeOptions { int defaultDelayMs{100}; int loopCount{0}; int targetWidth{0}; int targetHeight{0}; bool dither{false}; };
class GifCodec {
public:
    static Result<QByteArray> encodeAnimatedGif(const std::vector<GifFrame>& frames,const GifEncodeOptions& options = {});
    static StatusResult saveAnimatedGif(const std::vector<GifFrame>& frames,const QString& targetPath,const GifEncodeOptions& options = {},bool allowOverwrite = true);
    static Result<std::vector<GifFrame>> extractGifFrames(const QString& gifFilePath);
    static Result<QStringList> exportGifFramesToFolder(const QString& gifFilePath,const QString& outputDir,const QString& baseName = "frame",const QString& format = "png",bool allowOverwrite = true);
};
}
