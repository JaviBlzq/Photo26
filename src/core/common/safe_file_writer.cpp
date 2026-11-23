#include "safe_file_writer.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QScopeGuard>
#include <QUuid>
#include <exception>
#ifdef Q_OS_WIN
#include <QtCore/qt_windows.h>
#else
#include <unistd.h>
#include <cerrno>
#include <cstring>
#endif

namespace photo26::core {
namespace {

StatusResult prepare(const QString& path, const SafeWriteOptions& o) {
    if (path.isEmpty()) {
        return StatusResult::failure(ErrorCode::InvalidParameter, "Ruta de destino vacía");
    }
    if (!o.allowOverwrite && QFileInfo::exists(path)) {
        return StatusResult::failure(ErrorCode::ConflictDetected, "El destino ya existe", path);
    }
    if (o.createParentDirs && !QDir().mkpath(QFileInfo(path).absolutePath())) {
        return StatusResult::failure(ErrorCode::FileWriteError, "No se pudo crear el directorio", path);
    }
    return StatusResult::success();
}

QString makeUniqueSiblingPath(const QString& targetPath, const QString& tag, const QString& ext) {
    const QString dir = QFileInfo(targetPath).absolutePath();
    const QString token = QUuid::createUuid().toString(QUuid::Id128);
    const QString suffix = ext.isEmpty() ? QString() : (QStringLiteral(".") + ext);
    return QDir(dir).filePath(QStringLiteral(".photo26-%1-%2%3").arg(tag, token, suffix));
}

StatusResult publishStagedNoOverwrite(const QString& stagedPath, const QString& targetPath) {
#ifdef Q_OS_WIN
    const auto from = QDir::toNativeSeparators(QFileInfo(stagedPath).absoluteFilePath()).toStdWString();
    const auto to = QDir::toNativeSeparators(QFileInfo(targetPath).absoluteFilePath()).toStdWString();
    // Without MOVEFILE_REPLACE_EXISTING, Windows atomically refuses to overwrite an existing file.
    if (!MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_WRITE_THROUGH)) {
        const DWORD error = GetLastError();
        return StatusResult::failure(
            (error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS)
                ? ErrorCode::ConflictDetected
                : ErrorCode::FileWriteError,
            "No se pudo publicar el archivo sin sobrescribir",
            QString::number(error));
    }
    return StatusResult::success();
#else
    const QByteArray from = QFile::encodeName(QFileInfo(stagedPath).absoluteFilePath());
    const QByteArray to = QFile::encodeName(QFileInfo(targetPath).absoluteFilePath());
    if (::link(from.constData(), to.constData()) != 0) {
        const int error = errno;
        return StatusResult::failure(
            error == EEXIST ? ErrorCode::ConflictDetected : ErrorCode::FileWriteError,
            "No se pudo publicar el archivo sin sobrescribir",
            QString::fromLocal8Bit(std::strerror(error)));
    }
    ::unlink(from.constData());
    return StatusResult::success();
#endif
}

StatusResult copyFileToSaveFile(const QString& sourcePath, const QString& targetPath) {
    QFile in(sourcePath);
    if (!in.open(QIODevice::ReadOnly)) {
        return StatusResult::failure(ErrorCode::FileReadError, "No se pudo leer el resultado temporal", in.errorString());
    }
    QSaveFile out(targetPath);
    out.setDirectWriteFallback(false);
    if (!out.open(QIODevice::WriteOnly)) {
        return StatusResult::failure(ErrorCode::FileWriteError, "No se pudo abrir el destino", out.errorString());
    }
    while (!in.atEnd()) {
        const QByteArray block = in.read(1024 * 1024);
        if (block.isEmpty() && in.error() != QFileDevice::NoError) {
            out.cancelWriting();
            return StatusResult::failure(ErrorCode::FileReadError, "Error leyendo el archivo temporal", in.errorString());
        }
        if (!block.isEmpty() && out.write(block) != block.size()) {
            out.cancelWriting();
            return StatusResult::failure(ErrorCode::FileWriteError, "Error copiando al destino", out.errorString());
        }
    }
    in.close();
    if (!out.commit()) {
        return StatusResult::failure(ErrorCode::FileWriteError, "No se pudo confirmar el archivo", out.errorString());
    }
    return StatusResult::success();
}

} // namespace

StatusResult SafeFileWriter::writeBytes(const QString& path, const QByteArray& data, const SafeWriteOptions& o) {
    auto p = prepare(path, o);
    if (!p.isOk()) {
        return p;
    }

    if (o.allowOverwrite) {
        QSaveFile out(path);
        out.setDirectWriteFallback(false);
        if (!out.open(QIODevice::WriteOnly)) {
            return StatusResult::failure(ErrorCode::FileWriteError, "No se pudo abrir el destino", out.errorString());
        }
        if (!data.isEmpty() && out.write(data) != data.size()) {
            out.cancelWriting();
            return StatusResult::failure(ErrorCode::FileWriteError, "Error escribiendo el archivo", out.errorString());
        }
        if (!out.commit()) {
            return StatusResult::failure(ErrorCode::FileWriteError, "No se pudo confirmar el archivo", out.errorString());
        }
        return StatusResult::success();
    }

    const QString stagedPath = makeUniqueSiblingPath(path, QStringLiteral("publish"), QStringLiteral("tmp"));
    bool keepStaged = false;
    auto cleanup = qScopeGuard([&] {
        if (!keepStaged && QFileInfo::exists(stagedPath)) {
            QFile::remove(stagedPath);
        }
    });

    {
        QFile staged(stagedPath);
        if (!staged.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return StatusResult::failure(ErrorCode::FileWriteError, "No se pudo preparar el destino", staged.errorString());
        }
        if (!data.isEmpty() && staged.write(data) != data.size()) {
            return StatusResult::failure(ErrorCode::FileWriteError, "Error escribiendo el archivo temporal", staged.errorString());
        }
        if (!staged.flush()) {
            return StatusResult::failure(ErrorCode::FileWriteError, "Error volcando el archivo temporal", staged.errorString());
        }
        staged.close();
    }

    auto pub = publishStagedNoOverwrite(stagedPath, path);
    if (pub.isOk()) {
        keepStaged = true; // File was moved/published to destination
    }
    return pub;
}

StatusResult SafeFileWriter::writeWithCallback(
    const QString& path,
    const std::function<StatusResult(const QString&)>& fn,
    const SafeWriteOptions& o) {
    auto p = prepare(path, o);
    if (!p.isOk()) {
        return p;
    }
    if (!fn) {
        return StatusResult::failure(ErrorCode::InvalidParameter, "Escritor vacío");
    }

    const QString tempPath = makeUniqueSiblingPath(path, QStringLiteral("stage"), QFileInfo(path).suffix());
    bool publishedDirectly = false;
    auto cleanup = qScopeGuard([&] {
        if (!publishedDirectly && QFileInfo::exists(tempPath)) {
            QFile::remove(tempPath);
        }
    });

    StatusResult r;
    try {
        r = fn(tempPath);
    } catch (const std::exception& e) {
        return StatusResult::failure(ErrorCode::FileWriteError, "El escritor falló", QString::fromUtf8(e.what()));
    }
    if (!r.isOk()) {
        return r;
    }
    if (!QFileInfo::exists(tempPath)) {
        return StatusResult::failure(ErrorCode::FileReadError, "El escritor no generó el archivo temporal", tempPath);
    }

    if (!o.allowOverwrite) {
        auto pub = publishStagedNoOverwrite(tempPath, path);
        if (pub.isOk()) {
            publishedDirectly = true;
        }
        return pub;
    }

    return copyFileToSaveFile(tempPath, path);
}

QString SafeFileWriter::suggestOutputFileName(
    const QString& original,
    const QString& suffix,
    const QString& ext,
    const QString& directory,
    bool avoid) {
    QString e = ext;
    while (e.startsWith('.')) e.remove(0, 1);
    QString base = QFileInfo(original).completeBaseName();
    if (!suffix.isEmpty()) base += '_' + suffix;
    auto name = [&](int i) {
        return base + (i ? "_" + QString::number(i) : QString()) + (e.isEmpty() ? QString() : "." + e);
    };
    QString dir = directory.isEmpty() ? QFileInfo(original).path() : directory;
    int i = 0;
    if (avoid) {
        while (QFileInfo::exists(QDir(dir).filePath(name(i)))) ++i;
    }
    return (directory.isEmpty() && QFileInfo(original).path() == ".") ? name(i) : QDir(dir).filePath(name(i));
}

} // namespace photo26::core
