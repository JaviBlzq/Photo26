#pragma once
#include "../common/result.hpp"
#include "../common/cancellation.hpp"
namespace photo26::core {
struct ZipEntry { QString archivePath; QByteArray data; };
class ZipWriter {
public:
    static StatusResult createZipFromEntries(const QString& zipFilePath,const std::vector<ZipEntry>& entries,bool allowOverwrite = true);
    static StatusResult createZipFromFiles(const QString& zipFilePath,const std::vector<std::pair<QString,QString>>& diskPathToArchiveName,bool allowOverwrite = true,const CancellationToken& cancelToken = {});
};
}
