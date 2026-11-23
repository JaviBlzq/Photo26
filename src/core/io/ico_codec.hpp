#pragma once
#include "../common/result.hpp"
namespace photo26::core {
enum class IconFitMode { ContainProportional, CoverCrop, Stretch };
struct FaviconExportOptions {
    std::vector<int> icoSizes{16,32,48}; std::vector<int> pngSizes{16,32,48,180,192,512};
    QColor backgroundColor{Qt::transparent}; IconFitMode fitMode{IconFitMode::ContainProportional}; int paddingPercent{0};
    bool generateWebManifest{true}; QString appName{"Photo26 App"}; QString appShortName{"App"}; QColor themeColor{"#ffffff"};
};
struct FaviconBundleResult { QString icoFilePath; QStringList pngFilePaths; QString manifestFilePath; };
class IcoCodec {
public:
    static Result<QByteArray> encodeMultiResolutionIco(const QImage& source, const std::vector<int>& sizes = {16,32,48}, const QColor& background = Qt::transparent, IconFitMode fitMode = IconFitMode::ContainProportional, int paddingPercent = 0);
    static StatusResult saveMultiResolutionIco(const QImage& source, const QString& targetPath, const std::vector<int>& sizes = {16,32,48}, const QColor& background = Qt::transparent, IconFitMode fitMode = IconFitMode::ContainProportional, int paddingPercent = 0, bool allowOverwrite = true);
    static Result<std::vector<QImage>> readIcoEntries(const QString& icoPath);
    static Result<FaviconBundleResult> exportFaviconBundle(const QImage& source, const QString& outputDir, const FaviconExportOptions& options = {}, bool allowOverwrite = true);
};
}
