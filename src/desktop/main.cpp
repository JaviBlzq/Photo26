#include "main_window.hpp"
#include "widgets/theme_manager.hpp"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <iostream>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif

namespace {

QJsonObject captureMemoryMetrics() {
    QJsonObject obj;
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    pmc.cb = sizeof(pmc);
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
        obj.insert(QStringLiteral("workingSetBytes"), static_cast<qint64>(pmc.WorkingSetSize));
        obj.insert(QStringLiteral("peakWorkingSetBytes"), static_cast<qint64>(pmc.PeakWorkingSetSize));
        obj.insert(QStringLiteral("privateUsageBytes"), static_cast<qint64>(pmc.PrivateUsage));
    }
#endif
    return obj;
}

void writeReportFile(const QString& reportPath, const QJsonObject& payload) {
    if (reportPath.isEmpty()) {
        return;
    }
    QFile file(reportPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(payload).toJson(QJsonDocument::Indented));
        file.flush();
    }
}

} // namespace

int main(int argc, char* argv[]) {
    QElapsedTimer startupTimer;
    startupTimer.start();

    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("Photo26"));
    QApplication::setApplicationName(QStringLiteral("Photo26"));
    QApplication::setApplicationDisplayName(QStringLiteral("Photo26 v1.0"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Photo26 v1.0 - Caja de herramientas nativa para imágenes"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption selfTestOpt(
        QStringLiteral("self-test"),
        QStringLiteral("Ejecuta la rutina de autodiagnóstico GUI y finaliza con código 0 si tiene éxito."));
    QCommandLineOption idleMsOpt(
        QStringLiteral("idle-ms"),
        QStringLiteral("Mantiene la ventana principal abierta N milisegundos y cierra automáticamente (para profiling)."),
        QStringLiteral("ms"),
        QStringLiteral("0"));
    QCommandLineOption toolOpt(
        QStringLiteral("tool"),
        QStringLiteral("Abre directamente una herramienta por su identificador (ej. convert, crop, resize)."),
        QStringLiteral("toolId"));
    QCommandLineOption reportFileOpt(
        QStringLiteral("report-file"),
        QStringLiteral("Guarda el informe JSON de autodiagnóstico o profiling en la ruta indicada."),
        QStringLiteral("path"));

    parser.addOption(selfTestOpt);
    parser.addOption(idleMsOpt);
    parser.addOption(toolOpt);
    parser.addOption(reportFileOpt);
    parser.process(app);

    const auto savedTheme = photo26::desktop::ThemeManager::loadSavedTheme();
    photo26::desktop::ThemeManager::applyTheme(&app, savedTheme);

    photo26::desktop::MainWindow window;
    window.setWindowIcon(photo26::desktop::ThemeManager::createToolVectorIcon(
        QStringLiteral("app_logo"), QColor(45, 95, 138), 64));

    if (parser.isSet(toolOpt)) {
        window.activateTool(parser.value(toolOpt));
    }

    const QStringList positionalFiles = parser.positionalArguments();
    if (!positionalFiles.isEmpty()) {
        window.loadFilesIntoWorkspace(positionalFiles);
    }

    const QString reportPath = parser.value(reportFileOpt);

    if (parser.isSet(selfTestOpt)) {
        window.show();
        QApplication::processEvents();
        const qint64 startupMs = startupTimer.elapsed();
        QString report;
        const bool ok = window.runSelfTestRoutine(&report);
        const qint64 totalMs = startupTimer.elapsed();
        std::cout << report.toStdString() << std::endl;

        QJsonObject payload;
        payload.insert(QStringLiteral("mode"), QStringLiteral("self-test"));
        payload.insert(QStringLiteral("ok"), ok);
        payload.insert(QStringLiteral("startupMs"), startupMs);
        payload.insert(QStringLiteral("totalMs"), totalMs);
        payload.insert(QStringLiteral("memory"), captureMemoryMetrics());
        payload.insert(QStringLiteral("report"), report);
        writeReportFile(reportPath, payload);

        return ok ? 0 : 1;
    }

    window.show();
    QApplication::processEvents();
    const qint64 startupMs = startupTimer.elapsed();

    const int idleMs = parser.value(idleMsOpt).toInt();
    if (idleMs > 0) {
        QTimer::singleShot(idleMs, &app, [&, startupMs, idleMs, reportPath]() {
            QJsonObject payload;
            payload.insert(QStringLiteral("mode"), QStringLiteral("idle"));
            payload.insert(QStringLiteral("ok"), true);
            payload.insert(QStringLiteral("startupMs"), startupMs);
            payload.insert(QStringLiteral("idleMs"), idleMs);
            payload.insert(QStringLiteral("memory"), captureMemoryMetrics());
            writeReportFile(reportPath, payload);
            app.quit();
        });
    }

    return app.exec();
}
