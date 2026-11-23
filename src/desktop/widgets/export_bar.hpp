#pragma once

#include "../../core/common/types.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <optional>
#include <vector>

namespace photo26::desktop {

struct BatchExportFileItem {
    QString originalFilePath;
    QString proposedFileName;
    QString targetFilePath;
    bool hasConflict{false};
};

struct BatchFolderSaveRequest {
    QString targetDirectory;
    QString namingPattern{QStringLiteral("{name}_{suffix}.{ext}")};
    photo26::core::SaveOptions saveOptions;
    bool allowOverwrite{false};
    bool openFolderAfterSave{false};
};

class ExportBarWidget : public QWidget {
    Q_OBJECT

public:
    explicit ExportBarWidget(QWidget* parent = nullptr);
    ~ExportBarWidget() override = default;

    void setCurrentToolSuffix(const QString& suffixTag);
    QString currentToolSuffix() const noexcept { return currentToolSuffix_; }

    void setForcedFormat(const QString& formatId);
    QString forcedFormat() const noexcept { return forcedFormat_; }

    void setSelectedFormat(const QString& formatId);
    void setQuality(int quality1to100);

    void setSourceFiles(const QStringList& filePaths);
    const QStringList& sourceFiles() const noexcept { return sourceFiles_; }

    void setMultiOutputMode(bool hasMultipleOutputs, int itemCount = 1);
    bool isMultiOutputMode() const noexcept { return multiOutputMode_; }

    void setBusyState(
        bool busy,
        int completed = 0,
        int total = 100,
        const QString& statusText = {});
    bool isBusy() const noexcept { return isBusy_; }

    void setStatusSummary(const QString& summaryText);
    void setLastExportedPath(const QString& fileOrDirPath);
    QString lastExportedPath() const noexcept { return lastExportedPath_; }

    photo26::core::SaveOptions currentSaveOptions() const;

    static QString lastExportDirectory();
    static void saveLastExportDirectory(const QString& dirPath);

    static QString suggestDescriptiveFileName(
        const QString& originalPath,
        const QString& suffixTag,
        const QString& extension);

    static std::vector<BatchExportFileItem> buildBatchPreviewItems(
        const QStringList& sourceFiles,
        const QString& targetDirectory,
        const QString& namingPattern,
        const QString& suffixTag,
        const QString& extension);

    QString promptSingleSaveFilePath(const QString& originalFilePath = {});
    QString promptZipSaveFilePath(const QString& defaultBaseName = QStringLiteral("photo26_export"));
    std::optional<BatchFolderSaveRequest> promptMultiSaveFolderDialog(const QStringList& sourceFiles);

signals:
    void saveSingleRequested(const QString& targetFilePath, const photo26::core::SaveOptions& options);
    void saveMultipleRequested(const BatchFolderSaveRequest& request);
    void saveZipRequested(const QString& targetZipFilePath, const photo26::core::SaveOptions& options);
    void cancelRequested();
    void saveOptionsChanged(const photo26::core::SaveOptions& options);

private:
    void buildUi();
    void updateFormatDependentControls();
    QString currentFormatId() const;
    void openLastExportedLocation();

    QComboBox* formatCombo_ = nullptr;
    QLabel* qualityLabel_ = nullptr;
    QSlider* qualitySlider_ = nullptr;
    QSpinBox* qualitySpin_ = nullptr;
    QCheckBox* losslessCheck_ = nullptr;
    QCheckBox* preserveMetadataCheck_ = nullptr;

    QLabel* statusInfoLabel_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QPushButton* cancelBtn_ = nullptr;

    QPushButton* openLastFolderBtn_ = nullptr;
    QPushButton* saveAsBtn_ = nullptr;
    QPushButton* saveMultipleBtn_ = nullptr;
    QPushButton* saveZipBtn_ = nullptr;

    QString currentToolSuffix_{QStringLiteral("editada")};
    QString forcedFormat_;
    QStringList sourceFiles_;
    QString lastExportedPath_;
    bool multiOutputMode_{false};
    int multiItemCount_{1};
    bool isBusy_{false};
};

} // namespace photo26::desktop
