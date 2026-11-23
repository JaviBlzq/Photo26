#include "export_bar.hpp"

#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QSettings>
#include <QStandardPaths>
#include <QStyle>
#include <QTableWidget>
#include <QUrl>
#include <QVBoxLayout>
#include <algorithm>

namespace photo26::desktop {

namespace {

constexpr const char* kSettingsOrg = "Photo26";
constexpr const char* kSettingsApp = "Photo26Desktop";
constexpr const char* kLastExportDirKey = "export/lastDirectory";

struct FormatEntry {
    QString id;
    QString label;
    QString filter;
    bool supportsQuality;
    bool supportsLossless;
};

const std::vector<FormatEntry>& supportedExportFormats() {
    static const std::vector<FormatEntry> kFormats = {
        {QStringLiteral("png"),  QStringLiteral("PNG (.png)"),   QStringLiteral("Imagen PNG (*.png)"), true, true},
        {QStringLiteral("jpg"),  QStringLiteral("JPEG (.jpg)"),  QStringLiteral("Imagen JPEG (*.jpg *.jpeg)"), true, false},
        {QStringLiteral("webp"), QStringLiteral("WebP (.webp)"), QStringLiteral("Imagen WebP (*.webp)"), true, true},
        {QStringLiteral("heic"), QStringLiteral("Apple HEIC (.heic)"), QStringLiteral("Imagen Apple HEIC (*.heic *.heics)"), true, true},
        {QStringLiteral("heif"), QStringLiteral("HEIF (.heif)"), QStringLiteral("Imagen HEIF (*.heif *.hif *.heifs)"), true, true},
        {QStringLiteral("avif"), QStringLiteral("AVIF (.avif)"), QStringLiteral("Imagen AVIF (*.avif *.avis)"), true, true},
        {QStringLiteral("bmp"),  QStringLiteral("BMP (.bmp)"),   QStringLiteral("Mapa de bits BMP (*.bmp)"), false, true},
        {QStringLiteral("tiff"), QStringLiteral("TIFF (.tiff)"), QStringLiteral("Imagen TIFF (*.tiff *.tif)"), false, true},
        {QStringLiteral("ico"),  QStringLiteral("ICO (.ico)"),   QStringLiteral("Icono de Windows (*.ico)"), false, true},
        {QStringLiteral("icns"), QStringLiteral("Apple ICNS (.icns)"), QStringLiteral("Icono de macOS (*.icns)"), false, true},
        {QStringLiteral("gif"),  QStringLiteral("GIF (.gif)"),   QStringLiteral("Imagen GIF (*.gif)"), false, false},
        {QStringLiteral("pdf"),  QStringLiteral("PDF (.pdf)"),   QStringLiteral("Documento PDF (*.pdf)"), true, false}
    };
    return kFormats;
}

QString normalizeExtensionToFormatId(const QString& extLower) {
    if (extLower == QStringLiteral("jpeg") || extLower == QStringLiteral("jpg")) {
        return QStringLiteral("jpg");
    }
    if (extLower == QStringLiteral("tif") || extLower == QStringLiteral("tiff")) {
        return QStringLiteral("tiff");
    }
    if (extLower == QStringLiteral("heic") || extLower == QStringLiteral("heics")) {
        return QStringLiteral("heic");
    }
    if (extLower == QStringLiteral("heif") || extLower == QStringLiteral("hif") || extLower == QStringLiteral("heifs")) {
        return QStringLiteral("heif");
    }
    if (extLower == QStringLiteral("avif") || extLower == QStringLiteral("avis")) {
        return QStringLiteral("avif");
    }
    for (const auto& f : supportedExportFormats()) {
        if (f.id == extLower) {
            return f.id;
        }
    }
    return QString();
}

} // namespace

ExportBarWidget::ExportBarWidget(QWidget* parent)
    : QWidget(parent) {
    setObjectName(QStringLiteral("ExportBar"));
    setAttribute(Qt::WA_StyledBackground, true);
    buildUi();
    updateFormatDependentControls();
}

void ExportBarWidget::buildUi() {
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(14, 10, 14, 10);
    mainLayout->setSpacing(12);

    // 1. Format selector
    auto* formatLabel = new QLabel(QStringLiteral("Formato:"), this);
    formatCombo_ = new QComboBox(this);
    formatCombo_->setMinimumWidth(128);
    for (const auto& entry : supportedExportFormats()) {
        formatCombo_->addItem(entry.label, entry.id);
    }

    // 2. Quality slider + spinbox
    qualityLabel_ = new QLabel(QStringLiteral("Calidad:"), this);
    qualitySlider_ = new QSlider(Qt::Horizontal, this);
    qualitySlider_->setRange(1, 100);
    qualitySlider_->setValue(90);
    qualitySlider_->setFixedWidth(96);

    qualitySpin_ = new QSpinBox(this);
    qualitySpin_->setRange(1, 100);
    qualitySpin_->setValue(90);
    qualitySpin_->setSuffix(QStringLiteral("%"));

    // 3. Lossless & Metadata checkboxes
    losslessCheck_ = new QCheckBox(QStringLiteral("Sin pérdida"), this);
    losslessCheck_->setChecked(false);

    preserveMetadataCheck_ = new QCheckBox(QStringLiteral("Conservar metadatos"), this);
    preserveMetadataCheck_->setChecked(true);

    // 4. Status label & Progress bar + Cancel button
    statusInfoLabel_ = new QLabel(QStringLiteral("Listo para exportar"), this);
    statusInfoLabel_->setObjectName(QStringLiteral("MutedLabel"));

    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    progressBar_->setFixedWidth(150);
    progressBar_->setVisible(false);

    cancelBtn_ = new QPushButton(QStringLiteral("Cancelar"), this);
    cancelBtn_->setVisible(false);

    // 5. Open last output folder button
    openLastFolderBtn_ = new QPushButton(QStringLiteral("Abrir carpeta de salida"), this);
    openLastFolderBtn_->setEnabled(false);

    // 6. Export action buttons
    saveMultipleBtn_ = new QPushButton(QStringLiteral("Guardar archivos…"), this);
    saveZipBtn_ = new QPushButton(QStringLiteral("Guardar como ZIP…"), this);
    saveAsBtn_ = new QPushButton(QStringLiteral("Guardar como…"), this);
    saveAsBtn_->setObjectName(QStringLiteral("PrimaryButton"));
    saveAsBtn_->setMinimumWidth(142);

    saveMultipleBtn_->setVisible(false);
    saveZipBtn_->setVisible(false);

    mainLayout->addWidget(formatLabel);
    mainLayout->addWidget(formatCombo_);
    mainLayout->addWidget(qualityLabel_);
    mainLayout->addWidget(qualitySlider_);
    mainLayout->addWidget(qualitySpin_);
    mainLayout->addWidget(losslessCheck_);
    mainLayout->addWidget(preserveMetadataCheck_);
    mainLayout->addSpacing(8);
    mainLayout->addWidget(statusInfoLabel_, 1);
    mainLayout->addWidget(progressBar_);
    mainLayout->addWidget(cancelBtn_);
    mainLayout->addWidget(openLastFolderBtn_);
    mainLayout->addWidget(saveMultipleBtn_);
    mainLayout->addWidget(saveZipBtn_);
    mainLayout->addWidget(saveAsBtn_);

    // Connections
    connect(formatCombo_, &QComboBox::currentIndexChanged, this, [this](int) {
        updateFormatDependentControls();
        emit saveOptionsChanged(currentSaveOptions());
    });

    connect(qualitySlider_, &QSlider::valueChanged, this, [this](int val) {
        if (qualitySpin_->value() != val) {
            qualitySpin_->setValue(val);
        }
        emit saveOptionsChanged(currentSaveOptions());
    });

    connect(qualitySpin_, &QSpinBox::valueChanged, this, [this](int val) {
        if (qualitySlider_->value() != val) {
            qualitySlider_->setValue(val);
        }
        emit saveOptionsChanged(currentSaveOptions());
    });

    connect(losslessCheck_, &QCheckBox::toggled, this, [this](bool) {
        updateFormatDependentControls();
        emit saveOptionsChanged(currentSaveOptions());
    });

    connect(preserveMetadataCheck_, &QCheckBox::toggled, this, [this](bool) {
        emit saveOptionsChanged(currentSaveOptions());
    });

    connect(cancelBtn_, &QPushButton::clicked, this, &ExportBarWidget::cancelRequested);

    connect(openLastFolderBtn_, &QPushButton::clicked, this, &ExportBarWidget::openLastExportedLocation);

    connect(saveAsBtn_, &QPushButton::clicked, this, [this]() {
        const QString primarySource = sourceFiles_.isEmpty() ? QString() : sourceFiles_.first();
        const QString chosenPath = promptSingleSaveFilePath(primarySource);
        if (!chosenPath.isEmpty()) {
            emit saveSingleRequested(chosenPath, currentSaveOptions());
        }
    });

    connect(saveMultipleBtn_, &QPushButton::clicked, this, [this]() {
        const auto req = promptMultiSaveFolderDialog(sourceFiles_);
        if (req.has_value()) {
            emit saveMultipleRequested(*req);
        }
    });

    connect(saveZipBtn_, &QPushButton::clicked, this, [this]() {
        const QString baseName = currentToolSuffix_.isEmpty()
            ? QStringLiteral("photo26_export")
            : QStringLiteral("photo26_%1").arg(currentToolSuffix_);
        const QString zipPath = promptZipSaveFilePath(baseName);
        if (!zipPath.isEmpty()) {
            emit saveZipRequested(zipPath, currentSaveOptions());
        }
    });
}

QString ExportBarWidget::currentFormatId() const {
    if (!forcedFormat_.isEmpty()) {
        return forcedFormat_;
    }
    return formatCombo_ ? formatCombo_->currentData().toString() : QStringLiteral("png");
}

void ExportBarWidget::updateFormatDependentControls() {
    const QString fmt = currentFormatId();
    bool supportsQual = false;
    bool supportsLossless = false;

    for (const auto& entry : supportedExportFormats()) {
        if (entry.id == fmt) {
            supportsQual = entry.supportsQuality;
            supportsLossless = entry.supportsLossless;
            break;
        }
    }

    const bool toggleableLossless =
        fmt == QStringLiteral("webp") || fmt == QStringLiteral("heic") ||
        fmt == QStringLiteral("heif") || fmt == QStringLiteral("avif");
    losslessCheck_->setEnabled(supportsLossless && toggleableLossless);
    if (fmt == QStringLiteral("png") || fmt == QStringLiteral("bmp") ||
        fmt == QStringLiteral("tiff") || fmt == QStringLiteral("ico") || fmt == QStringLiteral("icns")) {
        losslessCheck_->setChecked(true);
    } else if (!supportsLossless) {
        losslessCheck_->setChecked(false);
    }

    const bool qualityEditable = supportsQual && !(toggleableLossless && losslessCheck_->isChecked());
    qualityLabel_->setEnabled(qualityEditable);
    qualitySlider_->setEnabled(qualityEditable);
    qualitySpin_->setEnabled(qualityEditable);
}

void ExportBarWidget::setCurrentToolSuffix(const QString& suffixTag) {
    currentToolSuffix_ = suffixTag.trimmed().isEmpty() ? QStringLiteral("editada") : suffixTag.trimmed();
}

void ExportBarWidget::setForcedFormat(const QString& formatId) {
    forcedFormat_ = formatId.trimmed().toLower();
    if (!forcedFormat_.isEmpty()) {
        setSelectedFormat(forcedFormat_);
        formatCombo_->setEnabled(false);
    } else {
        formatCombo_->setEnabled(!isBusy_);
    }
    updateFormatDependentControls();
}

void ExportBarWidget::setSelectedFormat(const QString& formatId) {
    const QString norm = formatId.trimmed().toLower();
    for (int i = 0; i < formatCombo_->count(); ++i) {
        if (formatCombo_->itemData(i).toString() == norm) {
            formatCombo_->setCurrentIndex(i);
            break;
        }
    }
    updateFormatDependentControls();
}

void ExportBarWidget::setQuality(int quality1to100) {
    const int clamped = std::clamp(quality1to100, 1, 100);
    qualitySpin_->setValue(clamped);
    qualitySlider_->setValue(clamped);
}

void ExportBarWidget::setSourceFiles(const QStringList& filePaths) {
    sourceFiles_ = filePaths;
}

void ExportBarWidget::setMultiOutputMode(bool hasMultipleOutputs, int itemCount) {
    multiOutputMode_ = hasMultipleOutputs;
    multiItemCount_ = std::max(1, itemCount);

    saveMultipleBtn_->setVisible(multiOutputMode_);
    saveZipBtn_->setVisible(multiOutputMode_);
    if (multiOutputMode_) {
        saveMultipleBtn_->setObjectName(QStringLiteral("PrimaryButton"));
        saveAsBtn_->setObjectName(QString());
    } else {
        saveAsBtn_->setObjectName(QStringLiteral("PrimaryButton"));
        saveMultipleBtn_->setObjectName(QString());
    }
    style()->unpolish(saveAsBtn_);
    style()->polish(saveAsBtn_);
    style()->unpolish(saveMultipleBtn_);
    style()->polish(saveMultipleBtn_);
}

void ExportBarWidget::setBusyState(
    bool busy,
    int completed,
    int total,
    const QString& statusText) {
    isBusy_ = busy;
    progressBar_->setVisible(busy);
    cancelBtn_->setVisible(busy);

    if (busy) {
        const int safeTotal = std::max(1, total);
        progressBar_->setRange(0, safeTotal);
        progressBar_->setValue(std::clamp(completed, 0, safeTotal));
        if (!statusText.isEmpty()) {
            statusInfoLabel_->setText(statusText);
        }
    } else if (!statusText.isEmpty()) {
        statusInfoLabel_->setText(statusText);
    }

    formatCombo_->setEnabled(!busy && forcedFormat_.isEmpty());
    saveAsBtn_->setEnabled(!busy);
    saveMultipleBtn_->setEnabled(!busy);
    saveZipBtn_->setEnabled(!busy);
}

void ExportBarWidget::setStatusSummary(const QString& summaryText) {
    statusInfoLabel_->setText(summaryText);
}

void ExportBarWidget::setLastExportedPath(const QString& fileOrDirPath) {
    lastExportedPath_ = fileOrDirPath;
    openLastFolderBtn_->setEnabled(!lastExportedPath_.isEmpty());
    if (!lastExportedPath_.isEmpty()) {
        const QFileInfo fi(lastExportedPath_);
        saveLastExportDirectory(fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath());
    }
}

photo26::core::SaveOptions ExportBarWidget::currentSaveOptions() const {
    photo26::core::SaveOptions opts;
    opts.format = currentFormatId();
    opts.quality = qualitySpin_->value();
    opts.lossless = losslessCheck_->isChecked();
    opts.preserveTransparency = (opts.format != QStringLiteral("jpg") && opts.format != QStringLiteral("bmp"));
    opts.backgroundColor = Qt::white;
    opts.preserveMetadata = preserveMetadataCheck_->isChecked();
    opts.preserveIccProfile = preserveMetadataCheck_->isChecked();
    return opts;
}

QString ExportBarWidget::lastExportDirectory() {
    QSettings settings(QString::fromLatin1(kSettingsOrg), QString::fromLatin1(kSettingsApp));
    QString saved = settings.value(QString::fromLatin1(kLastExportDirKey)).toString();
    if (!saved.isEmpty() && QDir(saved).exists()) {
        return saved;
    }
    QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (!pictures.isEmpty() && QDir(pictures).exists()) {
        return pictures;
    }
    return QDir::homePath();
}

void ExportBarWidget::saveLastExportDirectory(const QString& dirPath) {
    if (dirPath.isEmpty()) {
        return;
    }
    QSettings settings(QString::fromLatin1(kSettingsOrg), QString::fromLatin1(kSettingsApp));
    settings.setValue(QString::fromLatin1(kLastExportDirKey), dirPath);
    settings.sync();
}

QString ExportBarWidget::suggestDescriptiveFileName(
    const QString& originalPath,
    const QString& suffixTag,
    const QString& extension) {
    const QString cleanExt = extension.trimmed().startsWith(QLatin1Char('.'))
        ? extension.trimmed().mid(1).toLower()
        : extension.trimmed().toLower();
    const QString extWithDot = cleanExt.isEmpty() ? QStringLiteral(".png") : (QStringLiteral(".") + cleanExt);
    const QString cleanSuffix = suffixTag.trimmed();

    if (originalPath.trimmed().isEmpty()) {
        if (cleanSuffix == QStringLiteral("favicon") || cleanExt == QStringLiteral("ico")) {
            return QStringLiteral("favicon") + extWithDot;
        }
        if (cleanSuffix == QStringLiteral("combinada") || cleanSuffix == QStringLiteral("composicion")) {
            return QStringLiteral("composicion") + extWithDot;
        }
        if (!cleanSuffix.isEmpty()) {
            return QStringLiteral("imagen_%1%2").arg(cleanSuffix, extWithDot);
        }
        return QStringLiteral("imagen") + extWithDot;
    }

    const QFileInfo fi(originalPath);
    const QString base = fi.completeBaseName().isEmpty() ? QStringLiteral("imagen") : fi.completeBaseName();
    if (cleanSuffix.isEmpty()) {
        return base + extWithDot;
    }
    return QStringLiteral("%1_%2%3").arg(base, cleanSuffix, extWithDot);
}

std::vector<BatchExportFileItem> ExportBarWidget::buildBatchPreviewItems(
    const QStringList& sourceFiles,
    const QString& targetDirectory,
    const QString& namingPattern,
    const QString& suffixTag,
    const QString& extension) {
    std::vector<BatchExportFileItem> result;
    const QString cleanExt = extension.trimmed().startsWith(QLatin1Char('.'))
        ? extension.trimmed().mid(1).toLower()
        : extension.trimmed().toLower();
    const QString safeExt = cleanExt.isEmpty() ? QStringLiteral("png") : cleanExt;
    const QString safeSuffix = suffixTag.trimmed().isEmpty() ? QStringLiteral("lote") : suffixTag.trimmed();
    const QDir outDir(targetDirectory);

    const int count = std::max<int>(1, sourceFiles.size());
    result.reserve(static_cast<std::size_t>(count));

    for (int i = 0; i < count; ++i) {
        const QString srcPath = (i < sourceFiles.size()) ? sourceFiles[i] : QString();
        const QFileInfo fi(srcPath);
        const QString baseName = fi.completeBaseName().isEmpty()
            ? QStringLiteral("imagen_%1").arg(i + 1, 2, 10, QLatin1Char('0'))
            : fi.completeBaseName();

        QString fileName = namingPattern.trimmed();
        if (fileName.isEmpty()) {
            fileName = QStringLiteral("{name}_{suffix}.{ext}");
        }

        fileName.replace(QStringLiteral("{name}"), baseName);
        fileName.replace(QStringLiteral("{suffix}"), safeSuffix);
        fileName.replace(QStringLiteral("{num}"), QStringLiteral("%1").arg(i + 1, 2, 10, QLatin1Char('0')));
        fileName.replace(QStringLiteral("{ext}"), safeExt);

        if (QFileInfo(fileName).suffix().isEmpty()) {
            fileName += QStringLiteral(".") + safeExt;
        }

        const QString targetPath = outDir.filePath(fileName);
        BatchExportFileItem item;
        item.originalFilePath = srcPath;
        item.proposedFileName = fileName;
        item.targetFilePath = targetPath;
        item.hasConflict = QFileInfo::exists(targetPath);
        result.push_back(std::move(item));
    }

    return result;
}

QString ExportBarWidget::promptSingleSaveFilePath(const QString& originalFilePath) {
    const QString currentFmt = currentFormatId();
    const QString suggestedName = suggestDescriptiveFileName(originalFilePath, currentToolSuffix_, currentFmt);
    const QString initialDir = lastExportDirectory();
    const QString initialPath = QDir(initialDir).filePath(suggestedName);

    QStringList filterList;
    QString selectedFilter;

    for (const auto& entry : supportedExportFormats()) {
        if (!forcedFormat_.isEmpty() && entry.id != forcedFormat_) {
            continue;
        }
        filterList.append(entry.filter);
        if (entry.id == currentFmt) {
            selectedFilter = entry.filter;
        }
    }
    const QString allFilter = filterList.join(QStringLiteral(";;"));

    const QString chosen = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Guardar resultado como…"),
        initialPath,
        allFilter,
        &selectedFilter);

    if (chosen.isEmpty()) {
        return QString();
    }

    const QFileInfo fi(chosen);
    saveLastExportDirectory(fi.absolutePath());

    const QString chosenExt = fi.suffix().toLower();
    const QString matchedFmt = normalizeExtensionToFormatId(chosenExt);
    if (!matchedFmt.isEmpty() && forcedFormat_.isEmpty()) {
        setSelectedFormat(matchedFmt);
    }

    setLastExportedPath(chosen);
    return chosen;
}

QString ExportBarWidget::promptZipSaveFilePath(const QString& defaultBaseName) {
    const QString initialDir = lastExportDirectory();
    const QString base = defaultBaseName.trimmed().isEmpty()
        ? QStringLiteral("photo26_export")
        : defaultBaseName.trimmed();
    const QString fileName = base.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive)
        ? base
        : (base + QStringLiteral(".zip"));
    const QString initialPath = QDir(initialDir).filePath(fileName);

    const QString chosen = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Guardar lote como archivo ZIP…"),
        initialPath,
        QStringLiteral("Archivo comprimido ZIP (*.zip)"));

    if (chosen.isEmpty()) {
        return QString();
    }

    const QFileInfo fi(chosen);
    saveLastExportDirectory(fi.absolutePath());
    setLastExportedPath(chosen);
    return chosen;
}

std::optional<BatchFolderSaveRequest> ExportBarWidget::promptMultiSaveFolderDialog(
    const QStringList& sourceFiles) {
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Guardar múltiples archivos en carpeta"));
    dlg.resize(680, 460);

    auto* rootLayout = new QVBoxLayout(&dlg);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    // Folder picker row
    auto* dirRow = new QHBoxLayout();
    auto* dirLabel = new QLabel(QStringLiteral("Carpeta de destino:"), &dlg);
    auto* dirEdit = new QLineEdit(lastExportDirectory(), &dlg);
    auto* browseBtn = new QPushButton(QStringLiteral("Examinar…"), &dlg);
    dirRow->addWidget(dirLabel);
    dirRow->addWidget(dirEdit, 1);
    dirRow->addWidget(browseBtn);
    rootLayout->addLayout(dirRow);

    // Naming pattern row
    auto* patternRow = new QHBoxLayout();
    auto* patternLabel = new QLabel(QStringLiteral("Patrón de nombres:"), &dlg);
    auto* patternCombo = new QComboBox(&dlg);
    patternCombo->setEditable(true);
    patternCombo->addItem(QStringLiteral("{name}_{suffix}.{ext}"));
    patternCombo->addItem(QStringLiteral("{name}_{num}.{ext}"));
    patternCombo->addItem(QStringLiteral("{name}.{ext}"));
    patternRow->addWidget(patternLabel);
    patternRow->addWidget(patternCombo, 1);
    rootLayout->addLayout(patternRow);

    // Preview table
    auto* previewTable = new QTableWidget(&dlg);
    previewTable->setColumnCount(3);
    previewTable->setHorizontalHeaderLabels({
        QStringLiteral("Archivo original"),
        QStringLiteral("Nombre de salida"),
        QStringLiteral("Estado en destino")
    });
    previewTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    previewTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    previewTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    previewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    previewTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    rootLayout->addWidget(previewTable, 1);

    // Conflict warning & options
    auto* conflictWarnLabel = new QLabel(&dlg);
    conflictWarnLabel->setStyleSheet(QStringLiteral("color: #F59E0B; font-weight: 600;"));
    rootLayout->addWidget(conflictWarnLabel);

    auto* overwriteCheck = new QCheckBox(
        QStringLiteral("Permitir sobrescribir archivos existentes si hay conflicto"),
        &dlg);
    overwriteCheck->setChecked(false);

    auto* openAfterCheck = new QCheckBox(
        QStringLiteral("Abrir carpeta al finalizar"),
        &dlg);
    openAfterCheck->setChecked(true);

    rootLayout->addWidget(overwriteCheck);
    rootLayout->addWidget(openAfterCheck);

    auto* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        &dlg);
    buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Guardar aquí"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("Cancelar"));
    rootLayout->addWidget(buttonBox);

    const QString fmt = currentFormatId();
    auto refreshPreview = [&]() {
        const auto items = buildBatchPreviewItems(
            sourceFiles,
            dirEdit->text().trimmed(),
            patternCombo->currentText(),
            currentToolSuffix_,
            fmt);

        previewTable->setRowCount(static_cast<int>(items.size()));
        int conflictCount = 0;

        for (int r = 0; r < static_cast<int>(items.size()); ++r) {
            const auto& item = items[static_cast<std::size_t>(r)];
            const QString origName = item.originalFilePath.isEmpty()
                ? QStringLiteral("(Resultado generado #%1)").arg(r + 1)
                : QFileInfo(item.originalFilePath).fileName();

            auto* c0 = new QTableWidgetItem(origName);
            auto* c1 = new QTableWidgetItem(item.proposedFileName);
            auto* c2 = new QTableWidgetItem(
                item.hasConflict
                    ? QStringLiteral("Ya existe en destino")
                    : QStringLiteral("Nuevo archivo"));

            if (item.hasConflict) {
                ++conflictCount;
                c2->setForeground(QColor(245, 158, 11));
            } else {
                c2->setForeground(QColor(16, 185, 129));
            }

            previewTable->setItem(r, 0, c0);
            previewTable->setItem(r, 1, c1);
            previewTable->setItem(r, 2, c2);
        }

        if (conflictCount > 0) {
            conflictWarnLabel->setVisible(true);
            conflictWarnLabel->setText(
                QStringLiteral("Atención: %1 archivo(s) ya existen en la carpeta destino. "
                               "Si no marcas permitir sobrescribir, la exportación se detendrá para proteger los archivos existentes.")
                    .arg(conflictCount));
        } else {
            conflictWarnLabel->setVisible(false);
        }
    };

    connect(browseBtn, &QPushButton::clicked, &dlg, [&]() {
        const QString picked = QFileDialog::getExistingDirectory(
            &dlg,
            QStringLiteral("Seleccionar carpeta de salida"),
            dirEdit->text());
        if (!picked.isEmpty()) {
            dirEdit->setText(picked);
        }
    });

    connect(dirEdit, &QLineEdit::textChanged, &dlg, [&](const QString&) { refreshPreview(); });
    connect(patternCombo, &QComboBox::currentTextChanged, &dlg, [&](const QString&) { refreshPreview(); });
    connect(buttonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    refreshPreview();

    if (dlg.exec() != QDialog::Accepted) {
        return std::nullopt;
    }

    const QString chosenDir = dirEdit->text().trimmed();
    if (chosenDir.isEmpty()) {
        return std::nullopt;
    }

    saveLastExportDirectory(chosenDir);
    setLastExportedPath(chosenDir);

    BatchFolderSaveRequest req;
    req.targetDirectory = chosenDir;
    req.namingPattern = patternCombo->currentText().trimmed();
    req.saveOptions = currentSaveOptions();
    req.allowOverwrite = overwriteCheck->isChecked();
    req.openFolderAfterSave = openAfterCheck->isChecked();
    return req;
}

void ExportBarWidget::openLastExportedLocation() {
    if (lastExportedPath_.isEmpty()) {
        return;
    }
    const QFileInfo fi(lastExportedPath_);
    const QString dirPath = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
    if (QDir(dirPath).exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(dirPath));
    }
}

} // namespace photo26::desktop
