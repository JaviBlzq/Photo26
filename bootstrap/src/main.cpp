#include "bootstrap_window.hpp"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QTextStream>
#include <QTimer>

#include <chrono>
#include <optional>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

namespace {
using Clock = std::chrono::steady_clock;

struct MemoryUsage {
    qint64 workingSetBytes{0};
    qint64 privateBytes{0};
};

[[nodiscard]] MemoryUsage currentMemoryUsage() noexcept
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
                             sizeof(counters)) != FALSE) {
        return {static_cast<qint64>(counters.WorkingSetSize),
                static_cast<qint64>(counters.PrivateUsage)};
    }
#endif
    return {};
}

[[nodiscard]] qint64 elapsedMilliseconds(const Clock::time_point started) noexcept
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - started).count();
}

[[nodiscard]] QJsonObject makeReport(const QString& mode,
                                     const qint64 startupMs,
                                     const MemoryUsage memory)
{
    return {{QStringLiteral("mode"), mode},
            {QStringLiteral("startup_ms"), startupMs},
            {QStringLiteral("working_set_bytes"), memory.workingSetBytes},
            {QStringLiteral("working_set_kib"), memory.workingSetBytes / 1024},
            {QStringLiteral("private_usage_bytes"), memory.privateBytes},
            {QStringLiteral("private_usage_kib"), memory.privateBytes / 1024}};
}

[[nodiscard]] bool writeReport(const QString& path, const QJsonObject& report)
{
    const auto data = QJsonDocument(report).toJson(QJsonDocument::Indented);
    if (path.isEmpty()) {
        QTextStream(stdout) << data;
        return true;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream(stderr) << "No se pudo escribir el informe: " << file.errorString() << '\n';
        return false;
    }
    return file.write(data) == data.size();
}

[[nodiscard]] std::optional<int> parseIdleMilliseconds(const QCommandLineParser& parser,
                                                        const QCommandLineOption& option)
{
    if (!parser.isSet(option)) {
        return std::nullopt;
    }
    bool valid = false;
    const int value = parser.value(option).toInt(&valid);
    if (!valid || value < 0) {
        return std::nullopt;
    }
    return value;
}
}

int main(int argc, char* argv[])
{
    const auto processStarted = Clock::now();
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("photo26-bootstrap"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Bootstrap verificable de Photo26"));
    parser.addHelpOption();
    const QCommandLineOption selfTest(QStringLiteral("self-test"),
                                      QStringLiteral("Ejecuta una autoverificación y termina."));
    const QCommandLineOption idleMs(QStringLiteral("idle-ms"),
                                    QStringLiteral("Permanece en reposo N milisegundos."),
                                    QStringLiteral("N"));
    const QCommandLineOption reportFile(QStringLiteral("report-file"),
                                        QStringLiteral("Escribe el informe JSON en la ruta indicada."),
                                        QStringLiteral("path"));
    parser.addOptions({selfTest, idleMs, reportFile});
    parser.process(app);

    const bool hasIdleOption = parser.isSet(idleMs);
    const auto idleDuration = parseIdleMilliseconds(parser, idleMs);
    if (hasIdleOption && !idleDuration.has_value()) {
        QTextStream(stderr) << "--idle-ms requiere un entero no negativo.\n";
        return 2;
    }
    if (parser.isSet(selfTest) && hasIdleOption) {
        QTextStream(stderr) << "--self-test y --idle-ms son mutuamente excluyentes.\n";
        return 2;
    }

    BootstrapWindow window;
    window.show();

    if (parser.isSet(selfTest)) {
        QApplication::processEvents();
        const qint64 startupMs = elapsedMilliseconds(processStarted);
        window.actionButton()->click();
        QApplication::processEvents();
        const bool valid = window.windowTitle() == QStringLiteral("Photo26")
                           && window.clickCount() == 1;
        auto report = makeReport(QStringLiteral("self-test"), startupMs, currentMemoryUsage());
        report.insert(QStringLiteral("passed"), valid);
        if (!writeReport(parser.value(reportFile), report)) {
            return 3;
        }
        return valid ? 0 : 1;
    }

    if (idleDuration.has_value()) {
        QApplication::processEvents();
        const qint64 startupMs = elapsedMilliseconds(processStarted);
        QTimer::singleShot(*idleDuration, &app, [&app, &parser, &reportFile, startupMs] {
            const auto report = makeReport(QStringLiteral("idle"), startupMs, currentMemoryUsage());
            app.exit(writeReport(parser.value(reportFile), report) ? 0 : 3);
        });
        return app.exec();
    }

    return app.exec();
}

