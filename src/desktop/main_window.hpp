#pragma once

#include "widgets/export_bar.hpp"
#include "widgets/interactive_canvas.hpp"
#include "widgets/theme_manager.hpp"
#include "widgets/tool_card_grid.hpp"

#include "../core/color/icc_manager.hpp"
#include "../core/common/cancellation.hpp"
#include "../core/common/result.hpp"
#include "../core/common/safe_file_writer.hpp"
#include "../core/common/types.hpp"
#include "../core/io/format_registry.hpp"
#include "../core/io/gif_codec.hpp"
#include "../core/io/ico_codec.hpp"
#include "../core/io/image_io.hpp"
#include "../core/io/pdf_exporter.hpp"
#include "../core/io/zip_writer.hpp"
#include "../core/metadata/metadata_engine.hpp"
#include "../core/ops/batch_rename_op.hpp"
#include "../core/ops/color_adjust_op.hpp"
#include "../core/ops/combine_op.hpp"
#include "../core/ops/compress_web_op.hpp"
#include "../core/ops/crop_transform.hpp"
#include "../core/ops/palette_compare_bg_op.hpp"
#include "../core/ops/resize_op.hpp"
#include "../core/ops/spritesheet_op.hpp"
#include "../core/ops/watermark_text_op.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTabWidget>
#include <QToolBar>
#include <vector>

namespace photo26::desktop {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

    // Navigation & tool activation (testable from GUI smoke tests)
    void showHomeScreen();
    void activateTool(const QString& toolId);
    QString activeToolId() const noexcept { return activeToolId_; }
    bool isOnHomeScreen() const noexcept;

    // File loading & workspace state
    bool loadFilesIntoWorkspace(const QStringList& filePaths);
    void clearWorkspaceFiles();
    QStringList loadedFilePaths() const;
    int selectedFileIndex() const;
    void setSelectedFileIndex(int index);

    // Processing & preview evaluation
    void applyCurrentToolPreview();
    const QImage& currentProcessedImage() const noexcept { return currentProcessedImage_; }
    const QImage& currentOriginalImage() const noexcept { return currentOriginalImage_; }

    // Direct programmatic export helpers (used by ExportBarWidget and automated tests)
    photo26::core::StatusResult exportSingleProcessedToFile(
        const QString& targetFilePath,
        const photo26::core::SaveOptions& options,
        bool allowOverwrite = true);

    photo26::core::StatusResult exportMultipleProcessedToFolder(
        const BatchFolderSaveRequest& request);

    photo26::core::StatusResult exportMultipleProcessedToZip(
        const QString& targetZipPath,
        const photo26::core::SaveOptions& options,
        bool allowOverwrite = true);

    // Self-test helper for --self-test CLI mode
    bool runSelfTestRoutine(QString* outDiagnosticReport = nullptr);

    // Accessors for key child widgets
    ToolCardGridWidget* homeGridWidget() const noexcept { return homeGrid_; }
    InteractiveCanvasWidget* canvasWidget() const noexcept { return canvas_; }
    ExportBarWidget* exportBarWidget() const noexcept { return exportBar_; }

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    void buildMainUi();
    void buildTopToolBar();
    void buildToolWorkspacePage();
    void buildAllToolParameterPages();
    void setupKeyboardShortcuts();
    void restoreWindowPreferences();
    void saveWindowPreferences() const;

    void refreshFileListWidget();
    void loadSelectedFileIntoCanvas();
    void updateToolHeaderAndExportBar();
    photo26::core::Result<QImage> evaluateToolOnImage(
        const QString& toolId,
        const photo26::core::LoadedImage& loadedItem,
        int itemIndex = 0);

    void toggleTheme();
    void promptOpenFilesDialog();

    // Main stack: index 0 = Home screen, index 1 = Tool Workspace
    QStackedWidget* rootStack_ = nullptr;
    ToolCardGridWidget* homeGrid_ = nullptr;
    QWidget* workspacePage_ = nullptr;

    // Top bar controls (contextual, non-redundant)
    QToolBar* topToolBar_ = nullptr;
    QAction* homeAction_ = nullptr;
    QAction* toolTitleAction_ = nullptr;
    QLabel* activeToolTitleLabel_ = nullptr;
    QAction* splitCompareAction_ = nullptr;
    QAction* fitZoomAction_ = nullptr;
    QAction* themeToggleAction_ = nullptr;

    // Workspace left panel (file list)
    QListWidget* fileListWidget_ = nullptr;
    QLabel* fileCountLabel_ = nullptr;

    // Workspace center panel (interactive preview canvas)
    InteractiveCanvasWidget* canvas_ = nullptr;
    QLabel* canvasHeaderLabel_ = nullptr;

    // Workspace right panel (tool-specific parameter pages)
    QStackedWidget* paramStack_ = nullptr;
    std::map<QString, int> toolIdToParamPageIndex_;

    // Workspace bottom export bar (F20)
    ExportBarWidget* exportBar_ = nullptr;

    // Loaded images state
    std::vector<photo26::core::LoadedImage> loadedItems_;
    int currentFileIndex_ = -1;
    QString activeToolId_{QStringLiteral("convert")};
    AppTheme currentTheme_{AppTheme::LightClean};

    QImage currentOriginalImage_;
    QImage currentProcessedImage_;
    photo26::core::CancellationToken activeCancelToken_;

    // Tool-specific controls stored as members for direct evaluation
    // F01 Convert
    QComboBox* convTargetFormatCombo_ = nullptr;
    QComboBox* convColorSpaceCombo_ = nullptr;
    QSpinBox* convSvgWidthSpin_ = nullptr;
    QSpinBox* convSvgHeightSpin_ = nullptr;
    QColor convBgColor_{Qt::white};
    QTableWidget* convCapabilitiesTable_ = nullptr;

    // F02 Crop & Transform
    QComboBox* cropPresetCombo_ = nullptr;
    QDoubleSpinBox* cropCustomWSpin_ = nullptr;
    QDoubleSpinBox* cropCustomHSpin_ = nullptr;
    QCheckBox* cropRuleOfThirdsCheck_ = nullptr;
    QSpinBox* cropXSpin_ = nullptr;
    QSpinBox* cropYSpin_ = nullptr;
    QSpinBox* cropWSpin_ = nullptr;
    QSpinBox* cropHSpin_ = nullptr;
    int cropRotateQuadrants_ = 0;
    QDoubleSpinBox* cropFreeAngleSpin_ = nullptr;
    QCheckBox* cropFlipHCheck_ = nullptr;
    QCheckBox* cropFlipVCheck_ = nullptr;

    // F03 Resize
    QComboBox* resizeModeCombo_ = nullptr;
    QSpinBox* resizeWidthSpin_ = nullptr;
    QSpinBox* resizeHeightSpin_ = nullptr;
    QDoubleSpinBox* resizePercentSpin_ = nullptr;
    QCheckBox* resizeKeepAspectCheck_ = nullptr;
    QCheckBox* resizePreventUpscaleCheck_ = nullptr;
    QComboBox* resizeInterpCombo_ = nullptr;

    // F04 Combine
    QComboBox* combineLayoutCombo_ = nullptr;
    QSpinBox* combineColsSpin_ = nullptr;
    QSpinBox* combineSpacingSpin_ = nullptr;
    QSpinBox* combineMarginSpin_ = nullptr;
    QCheckBox* combineTransparentBgCheck_ = nullptr;
    QColor combineBgColor_{Qt::white};
    QComboBox* combineAlignCombo_ = nullptr;
    QComboBox* combineScalePolicyCombo_ = nullptr;

    // F05 Color Adjust
    QCheckBox* colorGrayscaleCheck_ = nullptr;
    QCheckBox* colorBwCheck_ = nullptr;
    QSpinBox* colorBwThresholdSpin_ = nullptr;
    QCheckBox* colorBwDitherCheck_ = nullptr;
    QCheckBox* colorSepiaCheck_ = nullptr;
    QDoubleSpinBox* colorSepiaIntensitySpin_ = nullptr;
    QCheckBox* colorInvertCheck_ = nullptr;
    QSlider* colorBrightnessSlider_ = nullptr;
    QSlider* colorContrastSlider_ = nullptr;
    QSlider* colorSaturationSlider_ = nullptr;
    QDoubleSpinBox* colorGammaSpin_ = nullptr;
    QDoubleSpinBox* colorExposureSpin_ = nullptr;

    // F06 Compress
    QComboBox* compFormatCombo_ = nullptr;
    QSpinBox* compQualitySpin_ = nullptr;
    QCheckBox* compLosslessCheck_ = nullptr;
    QSpinBox* compTargetKiBSpin_ = nullptr;
    QLabel* compMetricsLabel_ = nullptr;
    photo26::core::CompressionMetrics lastCompressionMetrics_;

    // F07 Web Optimizer
    QCheckBox* web320Check_ = nullptr;
    QCheckBox* web640Check_ = nullptr;
    QCheckBox* web960Check_ = nullptr;
    QCheckBox* web1280Check_ = nullptr;
    QCheckBox* web1920Check_ = nullptr;
    QComboBox* webFormatCombo_ = nullptr;
    QSpinBox* webQualitySpin_ = nullptr;
    QCheckBox* webStripMetaCheck_ = nullptr;
    QLineEdit* webPatternEdit_ = nullptr;
    QLabel* webSummaryLabel_ = nullptr;

    // F08 Favicon & Icons
    QComboBox* favFitModeCombo_ = nullptr;
    QSpinBox* favPaddingSpin_ = nullptr;
    QCheckBox* favTransparentCheck_ = nullptr;
    QColor favBgColor_{Qt::transparent};
    QCheckBox* favPwaManifestCheck_ = nullptr;
    QLineEdit* favAppNameEdit_ = nullptr;

    // F09 Metadata
    QTableWidget* metaFilePropsTable_ = nullptr;
    QLineEdit* metaAuthorEdit_ = nullptr;
    QLineEdit* metaCopyrightEdit_ = nullptr;
    QLineEdit* metaTitleEdit_ = nullptr;
    QLineEdit* metaDescEdit_ = nullptr;
    QLineEdit* metaCaptureDateEdit_ = nullptr;
    QLineEdit* metaModDateEdit_ = nullptr;
    QLineEdit* metaCameraEdit_ = nullptr;
    QLineEdit* metaLensEdit_ = nullptr;
    QLineEdit* metaSoftwareEdit_ = nullptr;
    QSpinBox* metaOrientationSpin_ = nullptr;
    QSpinBox* metaDpiXSpin_ = nullptr;
    QSpinBox* metaDpiYSpin_ = nullptr;
    QLineEdit* metaKeywordsEdit_ = nullptr;
    QLineEdit* metaLicenseEdit_ = nullptr;
    QLineEdit* metaGpsLatEdit_ = nullptr;
    QLineEdit* metaGpsLonEdit_ = nullptr;
    QLabel* metaColorProfileLabel_ = nullptr;
    QCheckBox* metaStripExifCheck_ = nullptr;
    QCheckBox* metaStripXmpCheck_ = nullptr;
    QCheckBox* metaStripIptcCheck_ = nullptr;
    QCheckBox* metaStripGpsCheck_ = nullptr;
    QCheckBox* metaStripIccCheck_ = nullptr;
    QLabel* metaWarningsLabel_ = nullptr;

    // F11 Watermark
    QCheckBox* wmUseImageCheck_ = nullptr;
    QLineEdit* wmTextEdit_ = nullptr;
    QSpinBox* wmFontSizeSpin_ = nullptr;
    QColor wmTextColor_{255, 255, 255, 220};
    QImage wmOverlayImage_;
    QComboBox* wmAnchorCombo_ = nullptr;
    QDoubleSpinBox* wmOpacitySpin_ = nullptr;
    QDoubleSpinBox* wmScaleSpin_ = nullptr;
    QSpinBox* wmMarginSpin_ = nullptr;
    QDoubleSpinBox* wmRotationSpin_ = nullptr;

    // F12 Text & Borders
    QLineEdit* tbTextEdit_ = nullptr;
    QSpinBox* tbFontSizeSpin_ = nullptr;
    QColor tbTextColor_{Qt::white};
    QComboBox* tbAnchorCombo_ = nullptr;
    QSpinBox* tbPaddingSpin_ = nullptr;
    QSpinBox* tbBorderWidthSpin_ = nullptr;
    QColor tbBorderColor_{Qt::black};
    QSpinBox* tbCornerRadiusSpin_ = nullptr;

    // F13 Animated GIF
    QSpinBox* gifDelaySpin_ = nullptr;
    QSpinBox* gifLoopSpin_ = nullptr;
    QCheckBox* gifDitherCheck_ = nullptr;
    QLabel* gifInfoLabel_ = nullptr;

    // F14 PDF
    QComboBox* pdfPageSizeCombo_ = nullptr;
    QComboBox* pdfOrientationCombo_ = nullptr;
    QDoubleSpinBox* pdfMarginSpin_ = nullptr;
    QSpinBox* pdfDpiSpin_ = nullptr;
    QCheckBox* pdfProportionalCheck_ = nullptr;

    // F15 Spritesheet
    QSpinBox* spriteColsSpin_ = nullptr;
    QSpinBox* spriteRowsSpin_ = nullptr;
    QSpinBox* spriteSpacingSpin_ = nullptr;
    QSpinBox* spritePaddingSpin_ = nullptr;
    QPlainTextEdit* spriteJsonPreview_ = nullptr;
    photo26::core::SpritesheetResult lastSpritesheetResult_;

    // F16 Palette
    QSpinBox* palCountSpin_ = nullptr;
    QTableWidget* palTable_ = nullptr;
    QComboBox* palExportFormatCombo_ = nullptr;
    std::vector<photo26::core::PaletteColor> currentPalette_;

    // F17 Compare
    QComboBox* cmpModeCombo_ = nullptr;
    QDoubleSpinBox* cmpSliderSpin_ = nullptr;
    QLabel* cmpMetricsLabel_ = nullptr;

    // F18 Background Removal
    QComboBox* bgMethodCombo_ = nullptr;
    QColor bgKeyColor_{Qt::white};
    QDoubleSpinBox* bgToleranceSpin_ = nullptr;
    QDoubleSpinBox* bgFeatherSpin_ = nullptr;
    QCheckBox* bgReplaceSolidCheck_ = nullptr;
    QColor bgReplacementColor_{Qt::white};
    QLabel* bgEngineNoteLabel_ = nullptr;

    // F19 Mass Rename
    QLineEdit* renPatternEdit_ = nullptr;
    QLineEdit* renPrefixEdit_ = nullptr;
    QLineEdit* renSuffixEdit_ = nullptr;
    QLineEdit* renFindEdit_ = nullptr;
    QLineEdit* renReplaceEdit_ = nullptr;
    QSpinBox* renStartNumSpin_ = nullptr;
    QSpinBox* renStepSpin_ = nullptr;
    QSpinBox* renZeroPadSpin_ = nullptr;
    QTableWidget* renPreviewTable_ = nullptr;

    // F10 Batch Processing
    QComboBox* batchFormatCombo_ = nullptr;
    QSpinBox* batchQualitySpin_ = nullptr;
    QCheckBox* batchLosslessCheck_ = nullptr;
    QCheckBox* batchStripMetaCheck_ = nullptr;
    QCheckBox* batchResizeCheck_ = nullptr;
    QSpinBox* batchResizeWSpin_ = nullptr;
    QSpinBox* batchResizeHSpin_ = nullptr;
    QCheckBox* batchColorCheck_ = nullptr;
    QCheckBox* batchWatermarkCheck_ = nullptr;
    QLineEdit* batchWatermarkTextEdit_ = nullptr;
    QLineEdit* batchRenamePatternEdit_ = nullptr;
    QPlainTextEdit* batchLogView_ = nullptr;
};

} // namespace photo26::desktop
