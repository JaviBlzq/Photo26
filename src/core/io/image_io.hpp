#pragma once
#include "../common/result.hpp"
#include "../metadata/metadata_engine.hpp"
namespace photo26::core {
struct LoadedImage { QImage image; QString sourcePath; QString detectedFormat; ImageMetadata metadata; FileSystemProperties fileProps; };
struct LoadOptions { ImageLimits limits{}; bool autoOrient{true}; bool readMetadata{true}; int svgWidth{0}; int svgHeight{0}; };
class ImageIo {
public:
    static Result<LoadedImage> loadImage(const QString& filePath,const LoadOptions& options = {});
    static StatusResult saveImage(const QImage& image,const QString& targetPath,const SaveOptions& options = {},const ImageMetadata* metadataToWrite = nullptr,bool allowOverwrite = true,QStringList* outWarnings = nullptr);
};
}
