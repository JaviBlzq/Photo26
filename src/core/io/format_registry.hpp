#pragma once
#include "../common/types.hpp"
#include <QByteArray>
namespace photo26::core {
class FormatRegistry {
public:
    static std::vector<FormatCapability> allFormats();
    static std::optional<FormatCapability> findFormat(const QString& formatOrExt);
    static QString detectFormatFromHeader(const QByteArray& headerBytes, const QString& fileNameHint = {});
    static QStringList supportedReadExtensions();
    static QStringList supportedWriteExtensions();
    static QString fileDialogReadFilter();
    static QString fileDialogWriteFilterFor(const QString& formatId);
};
}
