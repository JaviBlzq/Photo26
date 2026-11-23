#pragma once

#include "result.hpp"
#include <QByteArray>
#include <QString>
#include <functional>

namespace photo26::core {

struct SafeWriteOptions {
    bool allowOverwrite{false};
    bool createParentDirs{true};
};

class SafeFileWriter {
public:
    // Writes raw bytes atomically using QSaveFile.
    // Returns ConflictDetected if targetPath exists and allowOverwrite is false.
    static StatusResult writeBytes(
        const QString& targetPath,
        const QByteArray& data,
        const SafeWriteOptions& options = {});

    // Executes a custom writer callback on a temporary file path or QIODevice and commits atomically.
    static StatusResult writeWithCallback(
        const QString& targetPath,
        const std::function<StatusResult(const QString& tempFilePath)>& writerFn,
        const SafeWriteOptions& options = {});

    // Suggests a descriptive output file name (e.g., "foto_recortada.png") without colliding if avoidCollision is set.
    static QString suggestOutputFileName(
        const QString& originalFilePath,
        const QString& suffixTag,
        const QString& targetExtension,
        const QString& targetDirectory = {},
        bool avoidCollision = false);
};

} // namespace photo26::core
