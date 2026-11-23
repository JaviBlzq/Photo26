#pragma once
#include "../common/result.hpp"
namespace photo26::core {
struct GpsCoordinate { double latitude{0.0}; double longitude{0.0}; std::optional<double> altitude; bool isValid{false}; };
struct FileSystemProperties { QString fileName; QString filePath; std::uint64_t fileSizeBytes{0}; QString createdIso; QString modifiedIso; int width{0}; int height{0}; int bitDepth{0}; bool hasAlpha{false}; QString detectedFormat; };
struct ImageMetadata {
    QString author; QString copyright; QString title; QString description; QString captureDate; QString modificationDate; QString cameraMakeModel; QString lensModel; QString software;
    int orientation{1}; int dpiX{72}; int dpiY{72}; QStringList keywords; QString license; std::optional<GpsCoordinate> gps; QString colorProfileName; QByteArray iccProfileBytes;
    bool hasExif{false}; bool hasXmp{false}; bool hasIptc{false}; bool hasGps{false}; bool hasIcc{false}; std::map<QString,QString> extraTags;
};
struct MetadataInspection { FileSystemProperties fileProps; ImageMetadata embedded; QStringList warnings; };
struct MetadataStripOptions { bool stripExif{true}; bool stripXmp{true}; bool stripIptc{true}; bool stripGps{true}; bool stripIcc{false}; };
class MetadataEngine {
public:
    static Result<MetadataInspection> inspectFile(const QString& filePath);
    static StatusResult stripAllMetadata(const QString& sourcePath,const QString& destPath,bool allowOverwrite = true);
    static StatusResult stripSelectedMetadata(const QString& sourcePath,const QString& destPath,const MetadataStripOptions& options,bool allowOverwrite = true);
    static StatusResult writeMetadataToFile(const QString& filePath,const ImageMetadata& metadata,QStringList* outWarnings = nullptr);
    static StatusResult copyMetadataBetweenFiles(const QString& sourceWithMetaPath,const QString& targetImagePath,QStringList* outWarnings = nullptr);
    // JSON v1 stores all embedded fields and flags at the root; optional fileProps is observational.
    // ICC is base64, keywords an array, GPS an object or null, extraTags a string/string object.
    static Result<QByteArray> exportToJson(const ImageMetadata& metadata,const FileSystemProperties* optionalFileProps = nullptr);
    static Result<ImageMetadata> importFromJson(const QByteArray& jsonBytes);
    static StatusResult batchUpdateMetadata(const QStringList& filePaths,const ImageMetadata& patchFields,const QStringList& fieldNamesToApply,QStringList* outWarnings = nullptr);
    static QStringList checkFormatCompatibilityWarnings(const QString& formatId,const ImageMetadata& metadata);
    static QByteArray buildExifBytes(const ImageMetadata& metadata,QStringList* outWarnings = nullptr);
    static QByteArray buildXmpBytes(const ImageMetadata& metadata);
    static QImage applyExifOrientation(const QImage& image,int orientation);
};
}
