#include "main_window.hpp"
#include "../core/io/heif_codec.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QColorDialog>
#include <QColorSpace>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QEventLoop>
#include <QScopeGuard>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPointer>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSettings>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
#include <cmath>
#include <exception>
namespace photo26::desktop {
    using namespace photo26::core;
    namespace {
        QString canonicalTool(QString id) {
            return id == "web_opt" ? QStringLiteral("web") : id;
        }
        QString diagnostic(const OperationError& e) {
            return e.message + (e.detail.isEmpty() ? QString() : "\n" + e.detail);
        }
        void report(QWidget* parent, const StatusResult& r) {
            if (!r.isOk()) QMessageBox::warning(parent, QStringLiteral("Photo26 — Operación fallida"), diagnostic(r.error));
        }
        QString chooseFolder(QWidget* parent) {
            return QFileDialog::getExistingDirectory(parent, QStringLiteral("Elegir carpeta de destino"), ExportBarWidget::lastExportDirectory());
        }
        bool safeLeaf(const QString& name) {
            static const QRegularExpression forbidden(QStringLiteral(R"([<>:"/\\|?*\x00-\x1f])"));
            static const QRegularExpression reserved(QStringLiteral("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\\.|$)"), QRegularExpression::CaseInsensitiveOption);
            return !name.isEmpty() && name != "." && name != ".." && !name.endsWith('.') && !name.endsWith(' ')
            && !forbidden.match(name).hasMatch() && !reserved.match(name).hasMatch();
        }
        QStringList droppedFiles(const QMimeData* mime) {
            QStringList paths;
            if (mime) for (const auto& url : mime->urls()) if (url.isLocalFile() && QFileInfo(url.toLocalFile()).isFile()) paths << url.toLocalFile();
            return paths;
        }
        void fillTable(QTableWidget* table, const std::vector<QStringList>& rows) {
            table->setRowCount(int(rows.size()));
            for (int r = 0; r < int(rows.size()); ++r)
            for (int c = 0; c < qMin(table->columnCount(), int(rows[size_t(r)].size())); ++c)
            table->setItem(r, c, new QTableWidgetItem(rows[size_t(r)][c]));
        }
        // Form construction and signal wiring are kept local to this translation unit.
        struct Form {
            QWidget* body;
            QFormLayout* layout;
            QTimer* timer;
            explicit Form(QWidget* widget, QTimer* previewTimer) : body(widget), layout(new QFormLayout(widget)), timer(previewTimer) {
                layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
                layout->setRowWrapPolicy(QFormLayout::WrapLongRows);
            }
            void add(const QString& label, QWidget* widget, const char* name) {
                widget->setObjectName(QString::fromLatin1(name));
                layout->addRow(label, widget);
            }
            void spin(QSpinBox*& w, const char* name, const QString& label, int low, int high, int value) {
                w = new QSpinBox(body);
                w->setRange(low, high);
                w->setValue(value);
                add(label, w, name);
                QObject::connect(w, &QSpinBox::valueChanged, timer, [t = timer] {
                    t->start();
                });
            }
            void real(QDoubleSpinBox*& w, const char* name, const QString& label, double low, double high, double value, double step = .1) {
                w = new QDoubleSpinBox(body);
                w->setRange(low, high);
                w->setDecimals(3);
                w->setSingleStep(step);
                w->setValue(value);
                add(label, w, name);
                QObject::connect(w, &QDoubleSpinBox::valueChanged, timer, [t = timer] {
                    t->start();
                });
            }
            void combo(QComboBox*& w, const char* name, const QString& label, const QStringList& items, int index = 0) {
                w = new QComboBox(body);
                w->addItems(items);
                w->setCurrentIndex(index);
                add(label, w, name);
                QObject::connect(w, &QComboBox::currentIndexChanged, timer, [t = timer] {
                    t->start();
                });
            }
            void check(QCheckBox*& w, const char* name, const QString& label, bool value = false) {
                w = new QCheckBox(label, body);
                w->setChecked(value);
                add({}, w, name);
                QObject::connect(w, &QCheckBox::toggled, timer, [t = timer] {
                    t->start();
                });
            }
            void edit(QLineEdit*& w, const char* name, const QString& label, const QString& value = {}) {
                w = new QLineEdit(value, body);
                add(label, w, name);
                QObject::connect(w, &QLineEdit::textEdited, timer, [t = timer] {
                    t->start();
                });
            }
            void label(QLabel*& w, const QString& text) {
                w = new QLabel(text, body);
                w->setWordWrap(true);
                w->setTextFormat(Qt::PlainText);
                layout->addRow(w);
            }
            QTableWidget* table(const QStringList& headings) {
                auto* w = new QTableWidget(0, int(headings.size()), body);
                w->setHorizontalHeaderLabels(headings);
                w->setEditTriggers(QAbstractItemView::NoEditTriggers);
                w->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
                w->horizontalHeader()->setStretchLastSection(true);
                w->setMinimumHeight(180);
                layout->addRow(w);
                return w;
            }
            void button(const QString& text, QObject* context, const std::function<void()>& callback) {
                auto* b = new QPushButton(text, body);
                layout->addRow(b);
                QObject::connect(b, &QPushButton::clicked, context, callback);
            }
            void color(const QString& text, QColor& value, QWidget* context) {
                auto* b = new QPushButton(text, body);
                layout->addRow(b);
                QColor* destination = &value;
                QObject::connect(b, &QPushButton::clicked, context, [b, destination, t = timer, context] {
                    QColor c = QColorDialog::getColor(*destination, context, QStringLiteral("Elegir color"), QColorDialog::ShowAlphaChannel);
                    if (c.isValid()) {
                        *destination = c;
                        b->setText(QStringLiteral("Color: %1").arg(c.name(QColor::HexArgb)));
                        t->start();
                    }
                });
            }
        };
#define SPIN(member, ...) f.spin(member, #member, __VA_ARGS__)
#define REAL(member, ...) f.real(member, #member, __VA_ARGS__)
#define COMBO(member, ...) f.combo(member, #member, __VA_ARGS__)
#define CHECK(member, ...) f.check(member, #member, __VA_ARGS__)
#define EDIT(member, ...) f.edit(member, #member, __VA_ARGS__)
        QStringList anchors(bool tiled = true) {
            QStringList a = {
                "Arriba izquierda", "Arriba centro", "Arriba derecha", "Centro izquierda", "Centro", "Centro derecha", "Abajo izquierda", "Abajo centro", "Abajo derecha"
            };
            if (tiled) a << "Mosaico";
            return a;
        }
        template<class T> T* control(QWidget* root, const char* name) {
            return root->findChild<T*>(QString::fromLatin1(name));
        }
        ColorAdjustOptions colorOptions(QWidget* root) {
            ColorAdjustOptions o;
            o.grayscale = control<QCheckBox>(root, "colorGrayscaleCheck_")->isChecked();
            o.blackAndWhite = control<QCheckBox>(root, "colorBwCheck_")->isChecked();
            o.bwThreshold = control<QSpinBox>(root, "colorBwThresholdSpin_")->value();
            o.bwDither = control<QCheckBox>(root, "colorBwDitherCheck_")->isChecked();
            o.sepia = control<QCheckBox>(root, "colorSepiaCheck_")->isChecked();
            o.sepiaIntensity = control<QDoubleSpinBox>(root, "colorSepiaIntensitySpin_")->value();
            o.invert = control<QCheckBox>(root, "colorInvertCheck_")->isChecked();
            o.brightness = control<QSlider>(root, "colorBrightnessSlider_")->value();
            o.contrast = control<QSlider>(root, "colorContrastSlider_")->value();
            o.saturation = control<QSlider>(root, "colorSaturationSlider_")->value();
            o.gamma = control<QDoubleSpinBox>(root, "colorGammaSpin_")->value();
            o.exposureEv = control<QDoubleSpinBox>(root, "colorExposureSpin_")->value();
            return o;
        }
        void setCropRectangle(QWidget* root, InteractiveCanvasWidget* canvas, const QRect& rect) {
            canvas->setCropRectImageCoords(rect);
            const QRect actual=canvas->cropRectImageCoords();
            const std::pair<const char*,int> fields[]={
                {
                    "cropXSpin_",actual.x()
                },{
                    "cropYSpin_",actual.y()
                },{
                    "cropWSpin_",actual.width()
                },{
                    "cropHSpin_",actual.height()
                }
            };
            for(const auto& [name,value]:fields){
                auto* spin=control<QSpinBox>(root,name);
                QSignalBlocker block(spin);
                spin->setValue(value);
            }
        }
        BatchPreset batchPreset(QWidget* root) {
            BatchPreset p;
            p.name = "Ajustes de Photo26";
            p.outputFormat = control<QComboBox>(root, "batchFormatCombo_")->currentText();
            p.quality = control<QSpinBox>(root, "batchQualitySpin_")->value();
            p.lossless = control<QCheckBox>(root, "batchLosslessCheck_")->isChecked();
            p.stripMetadata = control<QCheckBox>(root, "batchStripMetaCheck_")->isChecked();
            p.enableResize = control<QCheckBox>(root, "batchResizeCheck_")->isChecked();
            p.resizeOptions.targetWidth = control<QSpinBox>(root, "batchResizeWSpin_")->value();
            p.resizeOptions.targetHeight = control<QSpinBox>(root, "batchResizeHSpin_")->value();
            p.enableColorAdjust = control<QCheckBox>(root, "batchColorCheck_")->isChecked();
            p.colorOptions = colorOptions(root);
            p.enableWatermark = control<QCheckBox>(root, "batchWatermarkCheck_")->isChecked();
            p.watermarkOptions.text = control<QLineEdit>(root, "batchWatermarkTextEdit_")->text();
            p.renameOptions.pattern = control<QLineEdit>(root, "batchRenamePatternEdit_")->text();
            // Preserve advanced preset settings that have no dedicated control in this header.
            const auto json = root->property("batchPresetJson").toByteArray();
            if (!json.isEmpty()) {
                auto previous = BatchProcessor::presetFromJson(QJsonDocument::fromJson(json).object());
                if (previous.isOk()) {
                    BatchPreset merged = *previous.value;
                    merged.outputFormat = p.outputFormat;
                    merged.quality = p.quality;
                    merged.lossless = p.lossless;
                    merged.stripMetadata = p.stripMetadata;
                    merged.enableResize = p.enableResize;
                    merged.resizeOptions.targetWidth = p.resizeOptions.targetWidth;
                    merged.resizeOptions.targetHeight = p.resizeOptions.targetHeight;
                    merged.enableColorAdjust = p.enableColorAdjust;
                    merged.colorOptions = p.colorOptions;
                    merged.enableWatermark = p.enableWatermark;
                    merged.watermarkOptions.text = p.watermarkOptions.text;
                    merged.renameOptions.pattern = p.renameOptions.pattern;
                    return merged;
                }
            }
            return p;
        }
        Result<ImageMetadata> metadataForm(QWidget* root, ImageMetadata m) {
            const std::pair<const char*, QString*> fields[] = {
                {
                    "metaAuthorEdit_", &m.author
                }, {
                    "metaCopyrightEdit_", &m.copyright
                }, {
                    "metaTitleEdit_", &m.title
                },
                {
                    "metaDescEdit_", &m.description
                }, {
                    "metaCaptureDateEdit_", &m.captureDate
                }, {
                    "metaModDateEdit_", &m.modificationDate
                },
                {
                    "metaCameraEdit_", &m.cameraMakeModel
                }, {
                    "metaLensEdit_", &m.lensModel
                }, {
                    "metaSoftwareEdit_", &m.software
                }, {
                    "metaLicenseEdit_", &m.license
                }
            };
            for (const auto& [name, out] : fields) *out = control<QLineEdit>(root, name)->text();
            m.orientation = control<QSpinBox>(root, "metaOrientationSpin_")->value();
            m.dpiX = control<QSpinBox>(root, "metaDpiXSpin_")->value();
            m.dpiY = control<QSpinBox>(root, "metaDpiYSpin_")->value();
            m.keywords = control<QLineEdit>(root, "metaKeywordsEdit_")->text().split(';', Qt::SkipEmptyParts);
            for (auto& word : m.keywords) word = word.trimmed();
            QString lat = control<QLineEdit>(root, "metaGpsLatEdit_")->text().trimmed(), lon = control<QLineEdit>(root, "metaGpsLonEdit_")->text().trimmed();
            if (lat.isEmpty() && lon.isEmpty()) {
                m.gps.reset();
                m.hasGps = false;
            } else {
                bool a = false, b = false;
                GpsCoordinate g = m.gps.value_or(GpsCoordinate{});
                g.latitude = lat.toDouble(&a);
                g.longitude = lon.toDouble(&b);
                if (!a || !b || !std::isfinite(g.latitude) || !std::isfinite(g.longitude) || std::abs(g.latitude) > 90 || std::abs(g.longitude) > 180)
                return Result<ImageMetadata>::failure(ErrorCode::InvalidParameter, "GPS: indique latitud entre −90 y 90 y longitud entre −180 y 180");
                g.isValid = true;
                m.gps = g;
                m.hasGps = true;
            }
            return Result<ImageMetadata>::success(m);
        }
        void populateMetadata(QWidget* root, const ImageMetadata& m) {
            const std::pair<const char*, QString> fields[] = {
                {
                    "metaAuthorEdit_", m.author
                }, {
                    "metaCopyrightEdit_", m.copyright
                }, {
                    "metaTitleEdit_", m.title
                },
                {
                    "metaDescEdit_", m.description
                }, {
                    "metaCaptureDateEdit_", m.captureDate
                }, {
                    "metaModDateEdit_", m.modificationDate
                },
                {
                    "metaCameraEdit_", m.cameraMakeModel
                }, {
                    "metaLensEdit_", m.lensModel
                }, {
                    "metaSoftwareEdit_", m.software
                },
                {
                    "metaLicenseEdit_", m.license
                }, {
                    "metaKeywordsEdit_", m.keywords.join("; ")
                },
                {
                    "metaGpsLatEdit_", m.gps ? QString::number(m.gps->latitude, 'g', 12) : QString()
                },
                {
                    "metaGpsLonEdit_", m.gps ? QString::number(m.gps->longitude, 'g', 12) : QString()
                }
            };
            for (const auto& [name, value] : fields) {
                auto* w = control<QLineEdit>(root, name);
                QSignalBlocker block(w);
                w->setText(value);
            }
            for (const auto& [name, value] : std::vector<std::pair<const char*, int>>{
                {
                    "metaOrientationSpin_",m.orientation
                },{
                    "metaDpiXSpin_",m.dpiX
                },{
                    "metaDpiYSpin_",m.dpiY
                }
            }) {
                auto* w = control<QSpinBox>(root, name);
                QSignalBlocker block(w);
                w->setValue(value);
            }
        }
        MassRenameOptions renameOptions(QWidget* root) {
            MassRenameOptions o;
            o.pattern = control<QLineEdit>(root,"renPatternEdit_")->text();
            o.prefix = control<QLineEdit>(root,"renPrefixEdit_")->text();
            o.suffix = control<QLineEdit>(root,"renSuffixEdit_")->text();
            o.findText = control<QLineEdit>(root,"renFindEdit_")->text();
            o.replaceText = control<QLineEdit>(root,"renReplaceEdit_")->text();
            o.startNumber = control<QSpinBox>(root,"renStartNumSpin_")->value();
            o.numberStep = control<QSpinBox>(root,"renStepSpin_")->value();
            o.zeroPadding = control<QSpinBox>(root,"renZeroPadSpin_")->value();
            return o;
        }
        WebOptimizerOptions webOptions(QWidget* root) {
            WebOptimizerOptions o;
            o.widths.clear();
            for (const auto& [name, width] : std::vector<std::pair<const char*, int>>{
                {
                    "web320Check_",320
                },{
                    "web640Check_",640
                },{
                    "web960Check_",960
                },{
                    "web1280Check_",1280
                },{
                    "web1920Check_",1920
                }
            })
            if (control<QCheckBox>(root,name)->isChecked()) o.widths.push_back(width);
            o.format = control<QComboBox>(root,"webFormatCombo_")->currentText();
            o.quality = control<QSpinBox>(root,"webQualitySpin_")->value();
            o.stripMetadata = control<QCheckBox>(root,"webStripMetaCheck_")->isChecked();
            o.namingPattern = control<QLineEdit>(root,"webPatternEdit_")->text();
            return o;
        }
        StatusResult saveBatchImage(const QImage& image,const QString& source,const QString& target,const BatchPreset& preset) {
            SaveOptions options;
            options.format=preset.outputFormat;
            options.quality=preset.quality;
            options.lossless=preset.lossless;
            options.backgroundColor=preset.backgroundColor;
            options.preserveMetadata=!preset.stripMetadata;
            ImageMetadata meta;
            QStringList warnings;
            if(!preset.stripMetadata){
                auto inspected=MetadataEngine::inspectFile(source);
                if(!inspected.isOk())return {
                    inspected.error
                };
                meta=inspected.value->embedded;
                meta.orientation=1;
            }
            return ImageIo::saveImage(image,target,options,preset.stripMetadata?nullptr:&meta,false,&warnings);
        }
        StatusResult emptyWorkspace() {
            return StatusResult::failure(ErrorCode::InvalidParameter, "Cargue una imagen antes de exportar");
        }
        QImage decodePreviewBytes(const QByteArray& bytes) {
            QImage img = QImage::fromData(bytes);
            if (!img.isNull()) return img;
            if (HeifCodec::isAvailable()) {
                auto dec = HeifCodec::decodeFromBytes(bytes);
                if (dec.isOk()) return std::move(dec.value->image);
            }
            return {};
        }
    }
    // namespace
    MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
        setObjectName("Photo26MainWindow");
        setWindowTitle("Photo26 — Herramientas de imagen");
        setAcceptDrops(true);
        currentTheme_ = ThemeManager::loadSavedTheme();
        buildMainUi();
        setupKeyboardShortcuts();
        restoreWindowPreferences();
        showHomeScreen();
    }
    void MainWindow::buildMainUi() {
        rootStack_ = new QStackedWidget(this);
        setCentralWidget(rootStack_);
        homeGrid_ = new ToolCardGridWidget(rootStack_);
        rootStack_->addWidget(homeGrid_);
        buildToolWorkspacePage();
        buildAllToolParameterPages();
        buildTopToolBar();
        connect(homeGrid_, &ToolCardGridWidget::toolActivated, this, &MainWindow::activateTool);
        connect(homeGrid_, &ToolCardGridWidget::filesDroppedOnHome, this, [this](const QStringList& files, const QString& tool) {
            activateTool(tool.isEmpty() ? QStringLiteral("convert") : tool);
            loadFilesIntoWorkspace(files);
        });
        connect(canvas_, &InteractiveCanvasWidget::filesDropped, this, [this](const QStringList& files) {
            loadFilesIntoWorkspace(files);
        });
        connect(canvas_, &InteractiveCanvasWidget::cropRectChanged, this, [this](const QRect& rect) {
            const QSignalBlocker a(cropXSpin_), b(cropYSpin_), c(cropWSpin_), d(cropHSpin_);
            cropXSpin_->setValue(rect.x());
            cropYSpin_->setValue(rect.y());
            cropWSpin_->setValue(rect.width());
            cropHSpin_->setValue(rect.height());
            if(activeToolId_=="crop")if(auto* timer=findChild<QTimer*>("previewDebounce"))timer->start();
        });
        connect(canvas_, &InteractiveCanvasWidget::colorPicked, this, [this](const QColor& color) {
            bgKeyColor_ = color;
            statusBar()->showMessage("Color seleccionado: " + color.name(), 5000);
            applyCurrentToolPreview();
        });
        auto outcome = [this](const StatusResult& r) {
            report(this, r);
            if (r.isOk()) statusBar()->showMessage("Exportación completada", 5000);
        };
        connect(exportBar_, &ExportBarWidget::saveSingleRequested, this, [this, outcome](const QString& path, const SaveOptions& options) {
            outcome(exportSingleProcessedToFile(path, options));
        });
        connect(exportBar_, &ExportBarWidget::saveMultipleRequested, this, [this, outcome](const BatchFolderSaveRequest& request) {
            outcome(exportMultipleProcessedToFolder(request));
        });
        connect(exportBar_, &ExportBarWidget::saveZipRequested, this, [this, outcome](const QString& path, const SaveOptions& options) {
            outcome(exportMultipleProcessedToZip(path, options));
        });
        connect(exportBar_, &ExportBarWidget::cancelRequested, this, [this] {
            activeCancelToken_.cancel();
        });
        connect(exportBar_, &ExportBarWidget::saveOptionsChanged, this, [this](const SaveOptions& options) {
            if (activeToolId_ == "convert") {
                const int index = convTargetFormatCombo_->findData(options.format);
                if (index >= 0) {
                    QSignalBlocker block(convTargetFormatCombo_);
                    convTargetFormatCombo_->setCurrentIndex(index);
                }
            }
            exportBar_->setStatusSummary("El resultado está preparado para guardar con los ajustes elegidos");
        });
    }
    void MainWindow::buildTopToolBar() {
        topToolBar_ = addToolBar("Navegación");
        topToolBar_->setObjectName("navigationToolbar");
        topToolBar_->setMovable(false);
        topToolBar_->setFloatable(false);

        homeAction_ = topToolBar_->addAction("← Herramientas", this, &MainWindow::showHomeScreen);

        activeToolTitleLabel_ = new QLabel(topToolBar_);
        activeToolTitleLabel_->setObjectName("ToolbarToolTitle");
        toolTitleAction_ = topToolBar_->addWidget(activeToolTitleLabel_);

        auto* spacer = new QWidget(topToolBar_);
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        topToolBar_->addWidget(spacer);

        splitCompareAction_ = topToolBar_->addAction("Antes / después");
        splitCompareAction_->setCheckable(true);
        connect(splitCompareAction_, &QAction::toggled, canvas_, &InteractiveCanvasWidget::setSplitComparisonEnabled);

        fitZoomAction_ = topToolBar_->addAction("Ajustar vista", canvas_, &InteractiveCanvasWidget::fitToWindow);

        themeToggleAction_ = topToolBar_->addAction(
            currentTheme_ == AppTheme::DarkModern ? "Modo claro" : "Modo oscuro",
            this,
            &MainWindow::toggleTheme);
    }
    void MainWindow::buildToolWorkspacePage() {
        workspacePage_ = new QWidget(rootStack_);
        auto* layout = new QVBoxLayout(workspacePage_);
        auto* split = new QSplitter(Qt::Horizontal, workspacePage_);
        split->setObjectName("workspaceSplitter");
        auto* files = new QWidget(split);
        auto* left = new QVBoxLayout(files);
        fileCountLabel_ = new QLabel("Sin archivos", files);
        left->addWidget(fileCountLabel_);
        fileListWidget_ = new QListWidget(files);
        fileListWidget_->setObjectName("workspaceFiles");
        left->addWidget(fileListWidget_);
        auto* add = new QPushButton("Añadir imágenes…", files);
        auto* clear = new QPushButton("Vaciar lista", files);
        left->addWidget(add);
        left->addWidget(clear);
        connect(add,&QPushButton::clicked,this,&MainWindow::promptOpenFilesDialog);
        connect(clear,&QPushButton::clicked,this,&MainWindow::clearWorkspaceFiles);
        connect(fileListWidget_, &QListWidget::currentRowChanged, this, &MainWindow::setSelectedFileIndex);
        auto* center = new QWidget(split);
        auto* middle = new QVBoxLayout(center);
        canvasHeaderLabel_ = new QLabel("Vista previa",center);
        middle->addWidget(canvasHeaderLabel_);
        canvas_ = new InteractiveCanvasWidget(center);
        canvas_->setMinimumSize(240,240);
        middle->addWidget(canvas_,1);
        paramStack_ = new QStackedWidget(split);
        paramStack_->setObjectName("toolParameterStack");
        paramStack_->setMinimumWidth(290);
        split->addWidget(files);
        split->addWidget(center);
        split->addWidget(paramStack_);
        split->setStretchFactor(1,1);
        split->setSizes({
            200,650,370
        });
        layout->addWidget(split,1);
        exportBar_ = new ExportBarWidget(workspacePage_);
        layout->addWidget(exportBar_);
        rootStack_->addWidget(workspacePage_);
    }
    void MainWindow::buildAllToolParameterPages() {
        auto* timer = new QTimer(this);
        timer->setObjectName("previewDebounce");
        timer->setSingleShot(true);
        timer->setInterval(180);
        connect(timer, &QTimer::timeout, this, &MainWindow::applyCurrentToolPreview);
        auto saveFolder = [this] {
            const QString dir = chooseFolder(this);
            if (dir.isEmpty()) return;
            BatchFolderSaveRequest r;
            r.targetDirectory = dir;
            r.saveOptions = exportBar_->currentSaveOptions();
            report(this,exportMultipleProcessedToFolder(r));
        };
        auto saveZip = [this] {
            const QString path = exportBar_->promptZipSaveFilePath();
            if (!path.isEmpty()) report(this,exportMultipleProcessedToZip(path,exportBar_->currentSaveOptions()));
        };
        for (const auto& tool : ToolCardGridWidget::allTools()) {
            auto* scroll = new QScrollArea(paramStack_);
            scroll->setWidgetResizable(true);
            scroll->setFrameShape(QFrame::NoFrame);
            scroll->setObjectName("parameters_" + tool.id);
            auto* body = new QWidget(scroll);
            Form f(body,timer);
            scroll->setWidget(body);
            toolIdToParamPageIndex_[tool.id] = paramStack_->addWidget(scroll);
            auto* title = new QLabel(tool.title,body);
            title->setWordWrap(true);
            f.layout->addRow(title);
            const QString id = canonicalTool(tool.id);
            if (id == "convert") {
                COMBO(convTargetFormatCombo_,"Formato de destino",{});
                COMBO(convColorSpaceCombo_,"Espacio de color",{
                    "Conservar perfil", "sRGB", "Adobe RGB", "Display P3", "ProPhoto RGB"
                });
                SPIN(convSvgWidthSpin_,"Ancho SVG (0 = original)",0,32768,0);
                SPIN(convSvgHeightSpin_,"Alto SVG (0 = original)",0,32768,0);
                f.color("Fondo para formatos sin transparencia",convBgColor_,this);
                convCapabilitiesTable_ = f.table({
                    "Formato","Lectura","Escritura","Alfa","Notas"
                });
                std::vector<QStringList> rows;
                for (const auto& cap : FormatRegistry::allFormats()) {
                    if (cap.canWrite) convTargetFormatCombo_->addItem(cap.displayName,cap.id);
                    rows.push_back({
                        cap.displayName,cap.canRead?"Sí":"No",cap.canWrite?"Sí":"No",cap.supportsAlpha?"Sí":"No",cap.notes
                    });
                }
                fillTable(convCapabilitiesTable_,rows);
                convTargetFormatCombo_->setCurrentIndex(convTargetFormatCombo_->findData("png"));
                connect(convTargetFormatCombo_,&QComboBox::currentIndexChanged,this,[this] {
                    exportBar_->setSelectedFormat(convTargetFormatCombo_->currentData().toString());
                });
            } else if (id == "crop") {
                COMBO(cropPresetCombo_,"Proporción",{
                    "Libre","1:1","3:4","4:3","16:9","9:16","2:3","3:2","21:9","Personalizada"
                });
                REAL(cropCustomWSpin_,"Proporción: ancho",.001,10000,1);
                REAL(cropCustomHSpin_,"Proporción: alto",.001,10000,1);
                CHECK(cropRuleOfThirdsCheck_,"Mostrar regla de los tercios",true);
                SPIN(cropXSpin_,"X",0,32768,0);
                SPIN(cropYSpin_,"Y",0,32768,0);
                SPIN(cropWSpin_,"Ancho",1,32768,1);
                SPIN(cropHSpin_,"Alto",1,32768,1);
                auto setRect = [this] {
                    if (currentOriginalImage_.isNull()) return;
                    QRect rect(cropXSpin_->value(),cropYSpin_->value(),cropWSpin_->value(),cropHSpin_->value());
                    rect = CropTransformOp::clampCropRect(rect,currentOriginalImage_.size(),static_cast<AspectRatioPreset>(cropPresetCombo_->currentIndex()),cropCustomWSpin_->value(),cropCustomHSpin_->value());
                    setCropRectangle(this,canvas_,rect);
                };
                for (auto* w : {
                    cropXSpin_,cropYSpin_,cropWSpin_,cropHSpin_
                }) connect(w,&QSpinBox::valueChanged,this,setRect);
                auto preset = [this] {
                    const auto p = static_cast<AspectRatioPreset>(cropPresetCombo_->currentIndex());
                    canvas_->setCropAspectRatio(CropTransformOp::aspectRatioValue(p,cropCustomWSpin_->value(),cropCustomHSpin_->value()));
                    if (!currentOriginalImage_.isNull()) setCropRectangle(this,canvas_,CropTransformOp::computeCenteredAspectCrop(currentOriginalImage_.size(),p,cropCustomWSpin_->value(),cropCustomHSpin_->value()));
                };
                connect(cropPresetCombo_,&QComboBox::currentIndexChanged,this,preset);
                connect(cropCustomWSpin_,&QDoubleSpinBox::valueChanged,this,preset);
                connect(cropCustomHSpin_,&QDoubleSpinBox::valueChanged,this,preset);
                connect(cropRuleOfThirdsCheck_,&QCheckBox::toggled,canvas_,&InteractiveCanvasWidget::setShowRuleOfThirds);
                for (int n : {
                    1,2,3
                }) f.button(QString("Rotar %1°").arg(n*90),this,[this,n] {
                    cropRotateQuadrants_ = n*90;
                    applyCurrentToolPreview();
                });
                f.button("Restablecer rotación",this,[this] {
                    cropRotateQuadrants_=0;
                    cropFreeAngleSpin_->setValue(0);
                    applyCurrentToolPreview();
                });
                REAL(cropFreeAngleSpin_,"Ángulo libre (°)",-360,360,0,1);
                CHECK(cropFlipHCheck_,"Voltear horizontalmente");
                CHECK(cropFlipVCheck_,"Voltear verticalmente");
            } else if (id == "resize") {
                COMBO(resizeModeCombo_,"Modo",{
                    "Píxeles","Porcentaje","Dimensiones máximas"
                });
                SPIN(resizeWidthSpin_,"Ancho",1,32768,1920);
                SPIN(resizeHeightSpin_,"Alto",1,32768,1080);
                REAL(resizePercentSpin_,"Porcentaje",.1,1000,100,1);
                CHECK(resizeKeepAspectCheck_,"Mantener proporción",true);
                CHECK(resizePreventUpscaleCheck_,"Evitar ampliaciones",true);
                COMBO(resizeInterpCombo_,"Interpolación",{
                    "Vecino más cercano","Bilineal","Bicúbica","Lanczos3"
                },3);
            } else if (id == "combine") {
                COMBO(combineLayoutCombo_,"Disposición",{
                    "Horizontal","Vertical","Cuadrícula","Mosaico"
                });
                SPIN(combineColsSpin_,"Columnas",1,100,2);
                SPIN(combineSpacingSpin_,"Espaciado",0,4096,8);
                SPIN(combineMarginSpin_,"Margen",0,4096,12);
                CHECK(combineTransparentBgCheck_,"Fondo transparente",true);
                f.color("Color de fondo",combineBgColor_,this);
                COMBO(combineAlignCombo_,"Alineación",{
                    "Inicio","Centro","Final"
                },1);
                COMBO(combineScalePolicyCombo_,"Ajuste de imágenes",{
                    "Tamaño original","Ajuste proporcional","Cubrir y recortar"
                },1);
            } else if (id == "color") {
                CHECK(colorGrayscaleCheck_,"Escala de grises");
                CHECK(colorBwCheck_,"Blanco y negro");
                SPIN(colorBwThresholdSpin_,"Umbral",0,255,128);
                CHECK(colorBwDitherCheck_,"Tramado en blanco y negro");
                CHECK(colorSepiaCheck_,"Sepia");
                REAL(colorSepiaIntensitySpin_,"Intensidad sepia",0,1,1);
                CHECK(colorInvertCheck_,"Invertir colores");
                for (const auto& [slot,label] : std::vector<std::pair<QSlider**,QString>>{
                    {
                        &colorBrightnessSlider_,"Brillo"
                    },{
                        &colorContrastSlider_,"Contraste"
                    },{
                        &colorSaturationSlider_,"Saturación"
                    }
                }) {
                    *slot = new QSlider(Qt::Horizontal,body);
                    (*slot)->setRange(-100,100);
                    f.layout->addRow(label,*slot);
                    connect(*slot,&QSlider::valueChanged,timer,[timer] {
                        timer->start();
                    });
                }
                colorBrightnessSlider_->setObjectName("colorBrightnessSlider_");
                colorContrastSlider_->setObjectName("colorContrastSlider_");
                colorSaturationSlider_->setObjectName("colorSaturationSlider_");
                REAL(colorGammaSpin_,"Gamma",.1,5,1);
                REAL(colorExposureSpin_,"Exposición (EV)",-5,5,0);
            } else if (id == "compress") {
                COMBO(compFormatCombo_,"Formato",{
                    "jpg","png","webp","heic","heif","avif"
                });
                SPIN(compQualitySpin_,"Calidad",1,100,80);
                CHECK(compLosslessCheck_,"Sin pérdidas (si el formato lo permite)");
                SPIN(compTargetKiBSpin_,"Objetivo KiB (0 = sin límite)",0,524288,0);
                f.label(compMetricsLabel_,"Cargue una imagen para comparar el tamaño");
            } else if (id == "web") {
                CHECK(web320Check_,"320 píxeles",true);
                CHECK(web640Check_,"640 píxeles",true);
                CHECK(web960Check_,"960 píxeles",true);
                CHECK(web1280Check_,"1280 píxeles",true);
                CHECK(web1920Check_,"1920 píxeles",true);
                COMBO(webFormatCombo_,"Formato",{
                    "webp","avif","heic","jpg","png"
                });
                SPIN(webQualitySpin_,"Calidad",1,100,82);
                CHECK(webStripMetaCheck_,"Eliminar metadatos",true);
                EDIT(webPatternEdit_,"Nombres","{name}-{width}w.{ext}");
                f.label(webSummaryLabel_,"Las variantes mantienen la proporción y evitan ampliaciones");
                f.button("Guardar variantes en carpeta…",this,saveFolder);
                f.button("Guardar variantes en ZIP…",this,saveZip);
            } else if (id == "favicon") {
                COMBO(favFitModeCombo_,"Ajuste",{
                    "Contener proporcionalmente","Cubrir y recortar","Estirar"
                });
                SPIN(favPaddingSpin_,"Margen interior (%)",0,49,0);
                CHECK(favTransparentCheck_,"Fondo transparente",true);
                f.color("Color de fondo",favBgColor_,this);
                CHECK(favPwaManifestCheck_,"Generar manifiesto web",true);
                EDIT(favAppNameEdit_,"Nombre de aplicación","Photo26 App");
                f.button("Exportar paquete completo a carpeta…",this,saveFolder);
                f.button("Exportar paquete completo a ZIP…",this,saveZip);
                f.button("Guardar favicon.ico…",this,[this] {
                    SaveOptions o=exportBar_->currentSaveOptions();
                    o.format="ico";
                    QString p=QFileDialog::getSaveFileName(this,"Guardar icono",ExportBarWidget::lastExportDirectory()+"/favicon.ico","Icono Windows (*.ico)");
                    if(!p.isEmpty()) report(this,exportSingleProcessedToFile(p,o));
                });
                f.button("Guardar icono macOS (.icns)…",this,[this] {
                    SaveOptions o=exportBar_->currentSaveOptions();
                    o.format="icns";
                    QString p=QFileDialog::getSaveFileName(this,"Guardar icono Apple ICNS",ExportBarWidget::lastExportDirectory()+"/icono.icns","Icono macOS (*.icns)");
                    if(!p.isEmpty()) report(this,exportSingleProcessedToFile(p,o));
                });
            } else if (id == "metadata") {
                auto* tabs = new QTabWidget(body);
                f.layout->addRow(tabs);
                auto* disk = new QWidget(tabs);
                Form diskForm(disk,timer);
                metaFilePropsTable_=diskForm.table({
                    "Propiedad","Valor"
                });
                tabs->addTab(disk,"Propiedades del archivo en disco");
                auto* embedded = new QWidget(tabs);
                tabs->addTab(embedded,"Metadatos incrustados");
                // Keep the field names stable so imports, copy and batch use the same form.
                Form ef(embedded,timer);
                ef.edit(metaAuthorEdit_,"metaAuthorEdit_","Autor");
                ef.edit(metaCopyrightEdit_,"metaCopyrightEdit_","Copyright");
                ef.edit(metaTitleEdit_,"metaTitleEdit_","Título");
                ef.edit(metaDescEdit_,"metaDescEdit_","Descripción");
                ef.edit(metaCaptureDateEdit_,"metaCaptureDateEdit_","Fecha de captura");
                ef.edit(metaModDateEdit_,"metaModDateEdit_","Fecha de modificación");
                ef.edit(metaCameraEdit_,"metaCameraEdit_","Cámara");
                ef.edit(metaLensEdit_,"metaLensEdit_","Objetivo");
                ef.edit(metaSoftwareEdit_,"metaSoftwareEdit_","Software");
                ef.spin(metaOrientationSpin_,"metaOrientationSpin_","Orientación EXIF",1,8,1);
                ef.spin(metaDpiXSpin_,"metaDpiXSpin_","DPI horizontal",1,1000000,72);
                ef.spin(metaDpiYSpin_,"metaDpiYSpin_","DPI vertical",1,1000000,72);
                ef.edit(metaKeywordsEdit_,"metaKeywordsEdit_","Palabras clave (separadas por ;)");
                ef.edit(metaLicenseEdit_,"metaLicenseEdit_","Licencia");
                ef.edit(metaGpsLatEdit_,"metaGpsLatEdit_","Latitud GPS");
                ef.edit(metaGpsLonEdit_,"metaGpsLonEdit_","Longitud GPS");
                ef.label(metaColorProfileLabel_,"Perfil ICC: sin perfil");
                f.label(metaWarningsLabel_,"Las operaciones de metadatos permiten guardar una copia");
                f.button("Cargar perfil ICC para los metadatos…",this,[this]{
                    if(currentFileIndex_<0)return;
                    const QString path=QFileDialog::getOpenFileName(this,"Cargar perfil ICC",{},"Perfiles ICC (*.icc *.icm)");
                    if(path.isEmpty())return;
                    QFile file(path);
                    if(!file.open(QIODevice::ReadOnly)||file.size()>16*1024*1024){
                        report(this,StatusResult::failure(ErrorCode::FileReadError,"No se pudo leer el perfil ICC o supera 16 MiB"));
                        return;
                    }
                    const QByteArray bytes=file.readAll();
                    const auto profile=QColorSpace::fromIccProfile(bytes);
                    if(!profile.isValid()){
                        report(this,StatusResult::failure(ErrorCode::InvalidParameter,"Perfil ICC inválido o no compatible"));
                        return;
                    }
                    auto& metadata=loadedItems_[size_t(currentFileIndex_)].metadata;
                    metadata.iccProfileBytes=bytes;
                    metadata.colorProfileName=profile.description();
                    metadata.hasIcc=true;
                    metaColorProfileLabel_->setText("Perfil ICC: "+profile.description());
                });
                CHECK(metaStripExifCheck_,"Eliminar EXIF",true);
                CHECK(metaStripXmpCheck_,"Eliminar XMP",true);
                CHECK(metaStripIptcCheck_,"Eliminar IPTC",true);
                CHECK(metaStripGpsCheck_,"Eliminar GPS",true);
                CHECK(metaStripIccCheck_,"Eliminar ICC");
                auto getMeta = [this]() -> Result<ImageMetadata> {
                    if(currentFileIndex_<0) return Result<ImageMetadata>::failure(ErrorCode::InvalidParameter,"Cargue una imagen");
                    return metadataForm(this,loadedItems_[size_t(currentFileIndex_)].metadata);
                };
                f.button("Guardar metadatos en una copia…",this,[this,getMeta] {
                    auto m=getMeta();
                    if(!m.isOk()){
                        report(this,{
                            m.error
                        });
                        return;
                    }
                    const auto& item=loadedItems_[size_t(currentFileIndex_)];
                    QString ext=QFileInfo(item.sourcePath).suffix();
                    QString dest=QFileDialog::getSaveFileName(this,"Guardar copia con metadatos",SafeFileWriter::suggestOutputFileName(item.sourcePath,"metadatos",ext));
                    if(dest.isEmpty())return;
                    QStringList warnings;
                    auto r=SafeFileWriter::writeWithCallback(dest,[&](const QString& temp) {
                        QFile in(item.sourcePath),out(temp);
                        if(!in.open(QIODevice::ReadOnly)||!out.open(QIODevice::WriteOnly|QIODevice::Truncate)) return StatusResult::failure(ErrorCode::FileWriteError,"No se pudo copiar el archivo");
                        while(!in.atEnd()){
                            auto bytes=in.read(1024*1024);
                            if(bytes.isEmpty()||out.write(bytes)!=bytes.size())return StatusResult::failure(ErrorCode::FileWriteError,"Error copiando la imagen");
                        }
                        out.close();
                        return MetadataEngine::writeMetadataToFile(temp,*m.value,&warnings);
                    },{
                        true,true
                    });
                    metaWarningsLabel_->setText(warnings.join('\n'));
                    report(this,r);
                });
                auto strip = [this](bool all) {
                    if(currentFileIndex_<0)return;
                    const auto& item=loadedItems_[size_t(currentFileIndex_)];
                    QString dest=QFileDialog::getSaveFileName(this,"Guardar copia sin metadatos",SafeFileWriter::suggestOutputFileName(item.sourcePath,"limpia",QFileInfo(item.sourcePath).suffix()));
                    if(dest.isEmpty())return;
                    MetadataStripOptions o{
                        metaStripExifCheck_->isChecked(),metaStripXmpCheck_->isChecked(),metaStripIptcCheck_->isChecked(),metaStripGpsCheck_->isChecked(),metaStripIccCheck_->isChecked()
                    };
                    report(this,all?MetadataEngine::stripAllMetadata(item.sourcePath,dest):MetadataEngine::stripSelectedMetadata(item.sourcePath,dest,o));
                };
                f.button("Eliminar todos los metadatos…",this,[strip]{
                    strip(true);
                });
                f.button("Eliminar categorías seleccionadas…",this,[strip]{
                    strip(false);
                });
                f.button("Copiar metadatos desde otra imagen…",this,[this] {
                    if(currentFileIndex_<0)return;
                    QString source=QFileDialog::getOpenFileName(this,"Copiar metadatos desde",{},FormatRegistry::fileDialogReadFilter());
                    if(source.isEmpty())return;
                    auto m=MetadataEngine::inspectFile(source);
                    if(!m.isOk()){
                        report(this,{
                            m.error
                        });
                        return;
                    }
                    loadedItems_[size_t(currentFileIndex_)].metadata=m.value->embedded;
                    populateMetadata(this,m.value->embedded);
                    metaWarningsLabel_->setText(m.value->warnings.join('\n'));
                });
                f.button("Exportar metadatos a JSON…",this,[this,getMeta] {
                    auto m=getMeta();
                    if(!m.isOk()){
                        report(this,{
                            m.error
                        });
                        return;
                    }
                    auto json=MetadataEngine::exportToJson(*m.value,&loadedItems_[size_t(currentFileIndex_)].fileProps);
                    if(!json.isOk()){
                        report(this,{
                            json.error
                        });
                        return;
                    }
                    QString path=QFileDialog::getSaveFileName(this,"Exportar metadatos",QDir(ExportBarWidget::lastExportDirectory()).filePath("metadatos.json"),"JSON (*.json)");
                    if(!path.isEmpty()){
                        ExportBarWidget::saveLastExportDirectory(QFileInfo(path).absolutePath());
                        report(this,SafeFileWriter::writeBytes(path,*json.value,{
                            true,true
                        }));
                    }
                });
                f.button("Importar metadatos desde JSON…",this,[this] {
                    if(currentFileIndex_<0)return;
                    QString path=QFileDialog::getOpenFileName(this,"Importar metadatos",ExportBarWidget::lastExportDirectory(),"JSON (*.json)");
                    if(path.isEmpty())return;
                    QFile file(path);
                    if(!file.open(QIODevice::ReadOnly)){
                        report(this,StatusResult::failure(ErrorCode::FileReadError,"No se pudo leer JSON"));
                        return;
                    }
                    auto m=MetadataEngine::importFromJson(file.readAll());
                    if(!m.isOk()){
                        report(this,{
                            m.error
                        });
                        return;
                    }
                    loadedItems_[size_t(currentFileIndex_)].metadata=*m.value;
                    populateMetadata(this,*m.value);
                });
                f.button("Aplicar campos editados a los archivos cargados…",this,[this,getMeta] {
                    auto m=getMeta();
                    if(!m.isOk()){
                        report(this,{
                            m.error
                        });
                        return;
                    }
                    if(QMessageBox::question(this,"Actualizar originales","Se modificarán los metadatos de todos los archivos cargados. ¿Continuar?")!=QMessageBox::Yes)return;
                    QStringList warnings;
                    const auto before=QJsonDocument::fromJson(property("metadataBaselineJson").toByteArray()).object();
                    const auto editedJson=MetadataEngine::exportToJson(*m.value);
                    if(!editedJson.isOk()){
                        report(this,{
                            editedJson.error
                        });
                        return;
                    }
                    const auto after=QJsonDocument::fromJson(*editedJson.value).object();
                    QStringList fields;
                    for(const auto& field:QStringList{
                        "author","copyright","title","description","captureDate","modificationDate","cameraMakeModel","lensModel","software","orientation","dpiX","dpiY","keywords","license","gps","iccProfileBytes","colorProfileName"
                    })
                    if(before[field]!=after[field])fields<<field;
                    if(fields.isEmpty()){
                        metaWarningsLabel_->setText("No hay campos editados que aplicar");
                        return;
                    }
                    auto r=MetadataEngine::batchUpdateMetadata(loadedFilePaths(),*m.value,fields,&warnings);
                    metaWarningsLabel_->setText(warnings.join('\n'));
                    report(this,r);
                    for(auto& item:loadedItems_){
                        auto inspection=MetadataEngine::inspectFile(item.sourcePath);
                        if(inspection.isOk()){
                            item.metadata=inspection.value->embedded;
                            item.fileProps=inspection.value->fileProps;
                        }
                    }
                    loadSelectedFileIntoCanvas();
                });
            } else if (id == "watermark") {
                CHECK(wmUseImageCheck_,"Usar imagen como marca");
                EDIT(wmTextEdit_,"Texto","© Photo26");
                SPIN(wmFontSizeSpin_,"Tamaño de letra",1,512,24);
                f.color("Color del texto",wmTextColor_,this);
                f.button("Elegir imagen de marca…",this,[this] {
                    QString path=QFileDialog::getOpenFileName(this,"Imagen de marca",{},FormatRegistry::fileDialogReadFilter());
                    if(path.isEmpty())return;
                    auto image=ImageIo::loadImage(path);
                    if(!image.isOk()){
                        report(this,{
                            image.error
                        });
                        return;
                    }
                    wmOverlayImage_=image.value->image;
                    wmUseImageCheck_->setChecked(true);
                    applyCurrentToolPreview();
                });
                COMBO(wmAnchorCombo_,"Posición",anchors(),8);
                REAL(wmOpacitySpin_,"Opacidad",0,1,.65);
                REAL(wmScaleSpin_,"Escala (%)",.1,1000,20,1);
                SPIN(wmMarginSpin_,"Margen",0,4096,20);
                REAL(wmRotationSpin_,"Rotación (°)",-360,360,0,1);
            } else if (id == "text_border") {
                EDIT(tbTextEdit_,"Texto superpuesto");
                SPIN(tbFontSizeSpin_,"Tamaño de letra",1,512,28);
                f.color("Color del texto",tbTextColor_,this);
                COMBO(tbAnchorCombo_,"Posición",anchors(false),7);
                SPIN(tbPaddingSpin_,"Relleno exterior",0,4096,0);
                SPIN(tbBorderWidthSpin_,"Grosor de borde",0,4096,0);
                f.color("Color de borde",tbBorderColor_,this);
                SPIN(tbCornerRadiusSpin_,"Radio de esquinas",0,4096,0);
            } else if (id == "gif") {
                SPIN(gifDelaySpin_,"Duración por fotograma (ms)",10,655350,100);
                SPIN(gifLoopSpin_,"Repeticiones (0 = infinito)",0,65535,0);
                CHECK(gifDitherCheck_,"Tramado de paleta");
                f.label(gifInfoLabel_,"Cada imagen cargada se convierte en un fotograma");
                f.button("Extraer fotogramas de un GIF…",this,[this] {
                    QString source=QFileDialog::getOpenFileName(this,"Abrir GIF animado",{},"GIF (*.gif)");
                    if(source.isEmpty())return;
                    QString dir=chooseFolder(this);
                    if(dir.isEmpty())return;
                    auto frames=GifCodec::exportGifFramesToFolder(source,dir,"fotograma","png",false);
                    if(!frames.isOk()){
                        report(this,{
                            frames.error
                        });
                        return;
                    }
                    gifInfoLabel_->setText(QString("Extraídos %1 fotogramas").arg(frames.value->size()));
                });
            } else if (id == "pdf") {
                COMBO(pdfPageSizeCombo_,"Página",{
                    "A4","Letter (carta)","Legal (oficio)","A3","Ajustar a imagen"
                });
                COMBO(pdfOrientationCombo_,"Orientación",{
                    "Vertical","Horizontal","Automática"
                });
                REAL(pdfMarginSpin_,"Márgenes (mm)",0,100,10,1);
                SPIN(pdfDpiSpin_,"Resolución (DPI)",36,2400,300);
                CHECK(pdfProportionalCheck_,"Ajuste proporcional",true);
            } else if (id == "spritesheet") {
                SPIN(spriteColsSpin_,"Columnas (0 = automático)",0,1000,0);
                SPIN(spriteRowsSpin_,"Filas (0 = automático)",0,1000,0);
                SPIN(spriteSpacingSpin_,"Espaciado",0,4096,2);
                SPIN(spritePaddingSpin_,"Margen interior",0,4096,2);
                spriteJsonPreview_=new QPlainTextEdit(body);
                spriteJsonPreview_->setReadOnly(true);
                spriteJsonPreview_->setMinimumHeight(180);
                f.layout->addRow("Mapa de coordenadas JSON",spriteJsonPreview_);
                f.button("Exportar atlas y JSON a carpeta…",this,saveFolder);
                f.button("Exportar atlas y JSON a ZIP…",this,saveZip);
            } else if (id == "palette") {
                SPIN(palCountSpin_,"Número de colores",1,256,6);
                palTable_=f.table({
                    "Muestra","HEX","RGB","Proporción"
                });
                COMBO(palExportFormatCombo_,"Formato de paleta",{
                    "JSON","Variables CSS","Paleta GIMP (GPL)","Muestras PNG"
                });
                auto copy = [this](bool hex) {
                    int row=palTable_->currentRow();
                    if(row<0||row>=int(currentPalette_.size()))return;
                    const auto& p=currentPalette_[size_t(row)];
                    QGuiApplication::clipboard()->setText(hex?p.hex:QString("rgb(%1, %2, %3)").arg(p.r).arg(p.g).arg(p.b));
                };
                f.button("Copiar HEX seleccionado",this,[copy]{
                    copy(true);
                });
                f.button("Copiar RGB seleccionado",this,[copy]{
                    copy(false);
                });
                f.button("Exportar paleta…",this,[this] {
                    auto bytes=PaletteExtractorOp::exportPalette(currentPalette_,static_cast<PaletteExportFormat>(palExportFormatCombo_->currentIndex()),"Paleta Photo26");
                    if(!bytes.isOk()){
                        report(this,{
                            bytes.error
                        });
                        return;
                    }
                    const QStringList ext{
                        "json","css","gpl","png"
                    };
                    QString path=QFileDialog::getSaveFileName(this,"Guardar paleta",QDir(ExportBarWidget::lastExportDirectory()).filePath("paleta."+ext[palExportFormatCombo_->currentIndex()]));
                    if(!path.isEmpty()){
                        ExportBarWidget::saveLastExportDirectory(QFileInfo(path).absolutePath());
                        report(this,SafeFileWriter::writeBytes(path,*bytes.value,{
                            true,true
                        }));
                    }
                });
            } else if (id == "compare") {
                COMBO(cmpModeCombo_,"Comparación",{
                    "Lado a lado","Deslizador dividido","Superposición","Diferencia visual"
                });
                REAL(cmpSliderSpin_,"División / mezcla",0,1,.5,.01);
                f.label(cmpMetricsLabel_,"Cargue dos imágenes o compare el original con el último resultado");
            } else if (id == "bg_remove") {
                COMBO(bgMethodCombo_,"Método",{
                    "Segmentación desde los bordes","Clave de color","Umbral de luminancia"
                });
                f.color("Color clave",bgKeyColor_,this);
                f.button("Seleccionar color en el lienzo",this,[this] {
                    canvas_->setColorPickerEnabled(true);
                    statusBar()->showMessage("Haga clic sobre el fondo de la imagen",5000);
                });
                REAL(bgToleranceSpin_,"Tolerancia",.01,.8,.18,.01);
                REAL(bgFeatherSpin_,"Suavizado del borde",0,100,1.5,.5);
                CHECK(bgReplaceSolidCheck_,"Sustituir por color sólido");
                f.color("Color de sustitución",bgReplacementColor_,this);
                f.label(bgEngineNoteLabel_,BackgroundRemovalOp::isOnnxRuntimeAvailable()?"Motor local ONNX disponible; estos controles usan segmentación local.":"Procesamiento 100 % local: segmentación por bordes, clave de color o luminancia. ONNX no está disponible; no se descargan modelos.");
            } else if (id == "rename") {
                EDIT(renPatternEdit_,"Patrón","{name}_{num}");
                EDIT(renPrefixEdit_,"Prefijo");
                EDIT(renSuffixEdit_,"Sufijo");
                EDIT(renFindEdit_,"Buscar");
                EDIT(renReplaceEdit_,"Sustituir por");
                SPIN(renStartNumSpin_,"Número inicial",0,1000000000,1);
                SPIN(renStepSpin_,"Incremento",1,1000000,1);
                SPIN(renZeroPadSpin_,"Dígitos",0,12,3);
                renPreviewTable_=f.table({
                    "Original","Propuesto","Estado"
                });
                auto execute = [this](bool copy) {
                    if(loadedItems_.empty())return;
                    QString dir=chooseFolder(this);
                    if(dir.isEmpty())return;
                    auto preview=MassRenameOp::previewRename(loadedFilePaths(),renameOptions(this),dir);
                    if(!copy&&QMessageBox::question(this,"Mover archivos","Se copiarán los archivos al destino y se eliminarán los originales tras cada copia correcta. ¿Continuar?")!=QMessageBox::Yes)return;
                    auto r=MassRenameOp::executeRenameOrCopy(preview,copy,false);
                    report(this,r);
                    if(r.isOk()&&!copy){
                        QStringList newPaths;
                        for(const auto& p:preview)newPaths<<p.proposedPath;
                        clearWorkspaceFiles();
                        loadFilesIntoWorkspace(newPaths);
                    }
                };
                f.button("Copiar con nombres propuestos…",this,[execute]{
                    execute(true);
                });
                f.button("Mover y renombrar…",this,[execute]{
                    execute(false);
                });
                f.button("Guardar originales renombrados en ZIP…",this,saveZip);
            } else if (id == "batch") {
                COMBO(batchFormatCombo_,"Formato",{
                    "webp","avif","heic","heif","jpg","png","tiff","bmp","ico","icns","gif","pdf"
                });
                SPIN(batchQualitySpin_,"Calidad",1,100,85);
                CHECK(batchLosslessCheck_,"Sin pérdidas");
                CHECK(batchStripMetaCheck_,"Eliminar metadatos",true);
                CHECK(batchResizeCheck_,"Redimensionar");
                SPIN(batchResizeWSpin_,"Ancho",1,32768,1920);
                SPIN(batchResizeHSpin_,"Alto",1,32768,1080);
                CHECK(batchColorCheck_,"Aplicar ajustes de color");
                CHECK(batchWatermarkCheck_,"Aplicar marca de agua");
                EDIT(batchWatermarkTextEdit_,"Texto de marca","© Photo26");
                EDIT(batchRenamePatternEdit_,"Patrón de nombres","{name}_lote_{num}");
                batchLogView_=new QPlainTextEdit(body);
                batchLogView_->setReadOnly(true);
                batchLogView_->setMinimumHeight(160);
                f.layout->addRow("Registro",batchLogView_);
                f.button("Guardar preset JSON…",this,[this] {
                    const auto p=batchPreset(this);
                    setProperty("batchPresetJson",QJsonDocument(BatchProcessor::presetToJson(p)).toJson());
                    QString path=QFileDialog::getSaveFileName(this,"Guardar preset",QDir(ExportBarWidget::lastExportDirectory()).filePath("preset_photo26.json"),"JSON (*.json)");
                    if(!path.isEmpty()){
                        ExportBarWidget::saveLastExportDirectory(QFileInfo(path).absolutePath());
                        report(this,SafeFileWriter::writeBytes(path,QJsonDocument(BatchProcessor::presetToJson(p)).toJson(),{
                            true,true
                        }));
                    }
                });
                f.button("Cargar preset JSON…",this,[this] {
                    QString path=QFileDialog::getOpenFileName(this,"Cargar preset",ExportBarWidget::lastExportDirectory(),"JSON (*.json)");
                    if(path.isEmpty())return;
                    QFile file(path);
                    if(!file.open(QIODevice::ReadOnly)){
                        report(this,StatusResult::failure(ErrorCode::FileReadError,"No se pudo abrir el preset"));
                        return;
                    }
                    QJsonParseError error;
                    auto json=QJsonDocument::fromJson(file.readAll(),&error);
                    if(error.error!=QJsonParseError::NoError||!json.isObject()){
                        report(this,StatusResult::failure(ErrorCode::SchemaValidationError,"JSON de preset inválido"));
                        return;
                    }
                    auto result=BatchProcessor::presetFromJson(json.object());
                    if(!result.isOk()){
                        report(this,{
                            result.error
                        });
                        return;
                    }
                    const auto& p=*result.value;
                    setProperty("batchPresetJson",json.toJson());
                    int index=batchFormatCombo_->findText(p.outputFormat);
                    if(index<0){
                        batchFormatCombo_->addItem(p.outputFormat);
                        index=batchFormatCombo_->count()-1;
                    }
                    batchFormatCombo_->setCurrentIndex(index);
                    batchQualitySpin_->setValue(p.quality);
                    batchLosslessCheck_->setChecked(p.lossless);
                    batchStripMetaCheck_->setChecked(p.stripMetadata);
                    batchResizeCheck_->setChecked(p.enableResize);
                    batchResizeWSpin_->setValue(p.resizeOptions.targetWidth);
                    batchResizeHSpin_->setValue(p.resizeOptions.targetHeight);
                    batchColorCheck_->setChecked(p.enableColorAdjust);
                    colorGrayscaleCheck_->setChecked(p.colorOptions.grayscale);
                    colorBwCheck_->setChecked(p.colorOptions.blackAndWhite);
                    colorBwThresholdSpin_->setValue(p.colorOptions.bwThreshold);
                    colorBwDitherCheck_->setChecked(p.colorOptions.bwDither);
                    colorSepiaCheck_->setChecked(p.colorOptions.sepia);
                    colorSepiaIntensitySpin_->setValue(p.colorOptions.sepiaIntensity);
                    colorInvertCheck_->setChecked(p.colorOptions.invert);
                    colorBrightnessSlider_->setValue(qRound(p.colorOptions.brightness));
                    colorContrastSlider_->setValue(qRound(p.colorOptions.contrast));
                    colorSaturationSlider_->setValue(qRound(p.colorOptions.saturation));
                    colorGammaSpin_->setValue(p.colorOptions.gamma);
                    colorExposureSpin_->setValue(p.colorOptions.exposureEv);
                    batchWatermarkCheck_->setChecked(p.enableWatermark);
                    batchWatermarkTextEdit_->setText(p.watermarkOptions.text);
                    batchRenamePatternEdit_->setText(p.renameOptions.pattern);
                    applyCurrentToolPreview();
                });
                f.button("Ejecutar lote en carpeta…",this,[this] {
                    if(loadedItems_.empty()||exportBar_->isBusy())return;
                    QString dir=chooseFolder(this);
                    if(dir.isEmpty())return;
                    BatchPreset p=batchPreset(this);
                    const QStringList paths=loadedFilePaths();
                    activeCancelToken_.reset();
                    const auto token=activeCancelToken_;
                    setProperty("batchActive",true);
                    exportBar_->setBusyState(true,0,int(paths.size()),"Procesando lote…");
                    workspacePage_->findChild<QSplitter*>()->setEnabled(false);
                    topToolBar_->setEnabled(false);
                    batchLogView_->clear();
                    auto* watcher=new QFutureWatcher<BatchSummary>(this);
                    QPointer<MainWindow> guarded(this);
                    const ProgressCallback progress=[guarded](int done,int total,const QString& path) {
                        if(!guarded)return;
                        QMetaObject::invokeMethod(guarded,[guarded,done,total,path] {
                            if(guarded && guarded->property("batchActive").toBool())guarded->exportBar_->setBusyState(true,done,total,"Procesado: "+QFileInfo(path).fileName());
                        },Qt::QueuedConnection);
                    };
                    connect(watcher,&QFutureWatcher<BatchSummary>::finished,this,[this,watcher,dir] {
                        try {
                            const auto summary=watcher->result();
                            for(const auto& r:summary.itemResults)batchLogView_->appendPlainText(QString("%1 — %2%3").arg(QFileInfo(r.sourcePath).fileName(),r.success?"Guardado":r.skipped?"Cancelado / omitido":"Error",r.errorMessage.isEmpty()?QString():" — "+r.errorMessage));
                            batchLogView_->appendPlainText(QString("Total: %1; correctos: %2; errores: %3; cancelados: %4").arg(summary.totalItems).arg(summary.succeeded).arg(summary.failed).arg(summary.cancelled));
                            exportBar_->setStatusSummary(QString("Lote: %1 correctos, %2 errores, %3 cancelados").arg(summary.succeeded).arg(summary.failed).arg(summary.cancelled));
                            exportBar_->setLastExportedPath(dir);
                        } catch(const std::exception& e) {
                            batchLogView_->appendPlainText("Error del lote: "+QString::fromUtf8(e.what()));
                        }
                        setProperty("batchActive",false);
                        exportBar_->setBusyState(false);
                        workspacePage_->findChild<QSplitter*>()->setEnabled(true);
                        topToolBar_->setEnabled(true);
                        watcher->deleteLater();
                    });
                    watcher->setFuture(QtConcurrent::run([paths,dir,p,token,progress] {
                        return BatchProcessor::runBatch(paths,dir,p,false,token,progress,0,saveBatchImage);
                    }));
                });
            }
            f.button("Actualizar vista previa",this,[this] {
                applyCurrentToolPreview();
            });
            f.layout->addItem(new QSpacerItem(0,12,QSizePolicy::Minimum,QSizePolicy::Expanding));
        }
        if(toolIdToParamPageIndex_.contains("web"))toolIdToParamPageIndex_["web_opt"]=toolIdToParamPageIndex_.at("web");
    }
#undef SPIN
#undef REAL
#undef COMBO
#undef CHECK
#undef EDIT
    void MainWindow::showHomeScreen() {
        if(exportBar_->isBusy())return;
        rootStack_->setCurrentIndex(0);
        canvas_->setCropOverlayEnabled(false);
        canvas_->setColorPickerEnabled(false);
        if(homeAction_)homeAction_->setVisible(false);
        if(toolTitleAction_)toolTitleAction_->setVisible(false);
        if(splitCompareAction_)splitCompareAction_->setVisible(false);
        if(fitZoomAction_)fitZoomAction_->setVisible(false);
    }
    bool MainWindow::isOnHomeScreen() const noexcept {
        return rootStack_ && rootStack_->currentIndex()==0;
    }
    void MainWindow::activateTool(const QString& requested) {
        if(exportBar_->isBusy())return;
        const QString id=canonicalTool(requested);
        const auto it=toolIdToParamPageIndex_.find(id);
        if(it==toolIdToParamPageIndex_.end()){
            statusBar()->showMessage("Herramienta desconocida: "+requested,5000);
            return;
        }
        if(activeToolId_!="compare"&&!currentProcessedImage_.isNull())setProperty("comparisonImage",QVariant::fromValue(currentProcessedImage_));
        activeToolId_=id;
        rootStack_->setCurrentIndex(1);
        paramStack_->setCurrentIndex(it->second);
        if(homeAction_)homeAction_->setVisible(true);
        if(toolTitleAction_)toolTitleAction_->setVisible(true);
        if(splitCompareAction_)splitCompareAction_->setVisible(true);
        if(fitZoomAction_)fitZoomAction_->setVisible(true);
        if(activeToolTitleLabel_){
            const auto tool=ToolCardGridWidget::findTool(id);
            activeToolTitleLabel_->setText(tool?tool->title:id);
        }
        canvas_->setColorPickerEnabled(false);
        canvas_->setCropOverlayEnabled(id=="crop");
        if(id=="crop"&&!currentOriginalImage_.isNull()){
            canvas_->setPreviewImage(currentOriginalImage_);
            const auto p=static_cast<AspectRatioPreset>(cropPresetCombo_->currentIndex());
            canvas_->setCropAspectRatio(CropTransformOp::aspectRatioValue(p,cropCustomWSpin_->value(),cropCustomHSpin_->value()));
            setCropRectangle(this,canvas_,CropTransformOp::computeCenteredAspectCrop(currentOriginalImage_.size(),p,cropCustomWSpin_->value(),cropCustomHSpin_->value()));
        }
        updateToolHeaderAndExportBar();
        applyCurrentToolPreview();
    }
    bool MainWindow::loadFilesIntoWorkspace(const QStringList& paths) {
        if(exportBar_->isBusy())return false;
        bool allOk=true;
        int firstAdded=-1;
        QStringList errors;
        for(const auto& path:paths){
            const QString absolute=QFileInfo(path).absoluteFilePath();
            auto found=std::find_if(loadedItems_.begin(),loadedItems_.end(),[&](const LoadedImage& item){
                return item.sourcePath==absolute;
            });
            if(found!=loadedItems_.end()){
                if(firstAdded<0)firstAdded=int(std::distance(loadedItems_.begin(),found));
                continue;
            }
            LoadOptions loadOptions;
            loadOptions.autoOrient=false;
            auto loaded=ImageIo::loadImage(path,loadOptions);
            if(!loaded.isOk()){
                allOk=false;
                errors<<QFileInfo(path).fileName()+": "+diagnostic(loaded.error);
                continue;
            }
            if(firstAdded<0)firstAdded=int(loadedItems_.size());
            // Keep disk EXIF values intact for the metadata editor; orient only the pixels (libheif already handles HEIF/AVIF transforms).
            const bool heifFmt=loaded.value->detectedFormat=="heic"||loaded.value->detectedFormat=="heif"||loaded.value->detectedFormat=="avif";
            if(!heifFmt&&loaded.value->metadata.orientation>1)loaded.value->image=MetadataEngine::applyExifOrientation(loaded.value->image,loaded.value->metadata.orientation);
            loadedItems_.push_back(std::move(*loaded.value));
        }
        refreshFileListWidget();
        if(firstAdded>=0){
            rootStack_->setCurrentIndex(1);
            if(homeAction_)homeAction_->setVisible(true);
            if(toolTitleAction_)toolTitleAction_->setVisible(true);
            if(splitCompareAction_)splitCompareAction_->setVisible(true);
            if(fitZoomAction_)fitZoomAction_->setVisible(true);
            setSelectedFileIndex(firstAdded);
        }
        updateToolHeaderAndExportBar();
        if(!errors.isEmpty()){
            exportBar_->setStatusSummary(errors.join(" | "));
            statusBar()->showMessage(errors.join(" | "),15000);
        }
        return allOk&&!paths.isEmpty()&&!loadedItems_.empty();
    }
    void MainWindow::clearWorkspaceFiles() {
        if(exportBar_->isBusy())return;
        loadedItems_.clear();
        currentFileIndex_=-1;
        currentOriginalImage_={};
        currentProcessedImage_={};
        currentPalette_.clear();
        lastSpritesheetResult_={};
        lastCompressionMetrics_={};
        setProperty("comparisonImage",QVariant());
        canvas_->clearImages();
        refreshFileListWidget();
        updateToolHeaderAndExportBar();
        spriteJsonPreview_->clear();
        palTable_->setRowCount(0);
        metaFilePropsTable_->setRowCount(0);
        renPreviewTable_->setRowCount(0);
        populateMetadata(this,{});
    }
    QStringList MainWindow::loadedFilePaths() const {
        QStringList paths;
        for(const auto& item:loadedItems_)paths<<item.sourcePath;
        return paths;
    }
    int MainWindow::selectedFileIndex() const {
        return currentFileIndex_;
    }
    void MainWindow::setSelectedFileIndex(int index) {
        if(exportBar_->isBusy()||index<0||index>=int(loadedItems_.size()))return;
        currentFileIndex_=index;
        {
            QSignalBlocker block(fileListWidget_);
            fileListWidget_->setCurrentRow(index);
        }
        loadSelectedFileIntoCanvas();
        updateToolHeaderAndExportBar();
    }
    void MainWindow::refreshFileListWidget() {
        QSignalBlocker block(fileListWidget_);
        fileListWidget_->clear();
        for(const auto& item:loadedItems_){
            auto* row=new QListWidgetItem(QFileInfo(item.sourcePath).fileName(),fileListWidget_);
            row->setToolTip(item.sourcePath);
        }
        fileCountLabel_->setText(QString("Archivos: %1").arg(loadedItems_.size()));
        if(currentFileIndex_>=0&&currentFileIndex_<int(loadedItems_.size()))fileListWidget_->setCurrentRow(currentFileIndex_);
        exportBar_->setSourceFiles(loadedFilePaths());
    }
    void MainWindow::loadSelectedFileIntoCanvas() {
        if(currentFileIndex_<0||currentFileIndex_>=int(loadedItems_.size()))return;
        const auto& item=loadedItems_[size_t(currentFileIndex_)];
        currentOriginalImage_=item.image;
        currentProcessedImage_=item.image;
        setProperty("comparisonImage",QVariant::fromValue(item.image));
        canvas_->setOriginalImage(item.image);
        canvas_->setPreviewImage(item.image);
        canvas_->fitToWindow();
        const auto preset=static_cast<AspectRatioPreset>(cropPresetCombo_->currentIndex());
        setCropRectangle(this,canvas_,CropTransformOp::computeCenteredAspectCrop(item.image.size(),preset,cropCustomWSpin_->value(),cropCustomHSpin_->value()));
        for(auto* spin:{
            resizeWidthSpin_,resizeHeightSpin_
        }){
            QSignalBlocker block(spin);
            spin->setValue(spin==resizeWidthSpin_?item.image.width():item.image.height());
        }
        populateMetadata(this,item.metadata);
        if(auto baseline=MetadataEngine::exportToJson(item.metadata);
        baseline.isOk())setProperty("metadataBaselineJson",*baseline.value);
        metaColorProfileLabel_->setText("Perfil ICC: "+(item.metadata.colorProfileName.isEmpty()?QStringLiteral("Sin perfil incrustado"):item.metadata.colorProfileName));
        const auto& p=item.fileProps;
        fillTable(metaFilePropsTable_,{
            {
                "Nombre",p.fileName
            },{
                "Ruta",p.filePath
            },{
                "Tamaño (bytes)",QString::number(p.fileSizeBytes)
            },{
                "Creación",p.createdIso
            },{
                "Modificación",p.modifiedIso
            },{
                "Dimensiones",QString("%1 × %2").arg(p.width).arg(p.height)
            },{
                "Profundidad",QString::number(p.bitDepth)
            },{
                "Canal alfa",p.hasAlpha?"Sí":"No"
            },{
                "Formato detectado",p.detectedFormat
            }
        });
        metaWarningsLabel_->setText(MetadataEngine::checkFormatCompatibilityWarnings(item.detectedFormat,item.metadata).join('\n'));
        applyCurrentToolPreview();
    }
    void MainWindow::updateToolHeaderAndExportBar() {
        const auto tool=ToolCardGridWidget::findTool(activeToolId_);
        const QString title=tool?tool->title:activeToolId_;
        if(activeToolTitleLabel_)activeToolTitleLabel_->setText(title);
        canvasHeaderLabel_->setText(currentOriginalImage_.isNull()?QStringLiteral("Vista previa"):QString("Vista previa — %1 × %2 px").arg(currentOriginalImage_.width()).arg(currentOriginalImage_.height()));
        exportBar_->setCurrentToolSuffix(tool?tool->defaultSuffix:QStringLiteral("editada"));
        exportBar_->setSourceFiles(loadedFilePaths());
        const bool many=loadedItems_.size()>1||activeToolId_=="web"||activeToolId_=="favicon"||activeToolId_=="spritesheet";
        exportBar_->setMultiOutputMode(many,qMax(1,int(loadedItems_.size())));
        exportBar_->setForcedFormat(activeToolId_=="gif"?"gif":activeToolId_=="pdf"?"pdf":activeToolId_=="favicon"?"ico":QString());
        if(activeToolId_=="convert")exportBar_->setSelectedFormat(convTargetFormatCombo_->currentData().toString());
        if(activeToolId_=="compress"){
            exportBar_->setSelectedFormat(compFormatCombo_->currentText());
            exportBar_->setQuality(compQualitySpin_->value());
        }
        gifInfoLabel_->setText(QString("%1 fotogramas; duración total aproximada: %2 ms").arg(loadedItems_.size()).arg(qint64(loadedItems_.size())*gifDelaySpin_->value()));
    }
    Result<QImage> MainWindow::evaluateToolOnImage(const QString& requested,const LoadedImage& item,int itemIndex) {
        const QString id=canonicalTool(requested);
        const QImage& image=item.image;
        if(image.isNull())return Result<QImage>::failure(ErrorCode::InvalidParameter,"Imagen vacía");
        if(id=="convert"){
            QImage source=image;
            if((item.detectedFormat=="svg"||item.detectedFormat=="svgz")&&(convSvgWidthSpin_->value()||convSvgHeightSpin_->value())){
                LoadOptions o;
                o.svgWidth=convSvgWidthSpin_->value();
                o.svgHeight=convSvgHeightSpin_->value();
                auto loaded=ImageIo::loadImage(item.sourcePath,o);
                if(!loaded.isOk())return Result<QImage>::failure(loaded.error.code,loaded.error.message,loaded.error.detail);
                source=loaded.value->image;
            }
            return IccManager::convertToColorSpace(source,static_cast<TargetColorSpace>(convColorSpaceCombo_->currentIndex()));
        }
        if(id=="crop"){
            CropOptions o;
            o.preset=static_cast<AspectRatioPreset>(cropPresetCombo_->currentIndex());
            o.customAspectWidth=cropCustomWSpin_->value();
            o.customAspectHeight=cropCustomHSpin_->value();
            QRect requestedRect(cropXSpin_->value(),cropYSpin_->value(),cropWSpin_->value(),cropHSpin_->value());
            o.cropRect=CropTransformOp::clampCropRect(requestedRect,image.size(),o.preset,o.customAspectWidth,o.customAspectHeight);
            auto result=CropTransformOp::crop(image,o);
            if(!result.isOk())return result;
            TransformOptions t;
            t.rotateQuadrants90=cropRotateQuadrants_;
            t.freeAngleDegrees=cropFreeAngleSpin_->value();
            t.flipHorizontal=cropFlipHCheck_->isChecked();
            t.flipVertical=cropFlipVCheck_->isChecked();
            return CropTransformOp::transform(*result.value,t);
        }
        if(id=="resize"){
            ResizeOptions o;
            o.mode=static_cast<ResizeMode>(resizeModeCombo_->currentIndex());
            o.targetWidth=resizeWidthSpin_->value();
            o.targetHeight=resizeHeightSpin_->value();
            o.maxWidth=o.targetWidth;
            o.maxHeight=o.targetHeight;
            o.percentage=resizePercentSpin_->value();
            o.keepAspectRatio=resizeKeepAspectCheck_->isChecked();
            o.preventUpscaling=resizePreventUpscaleCheck_->isChecked();
            o.interpolation=static_cast<InterpolationMethod>(resizeInterpCombo_->currentIndex());
            return ResizeOp::resize(image,o);
        }
        if(id=="combine"){
            std::vector<QImage> images;
            for(const auto& loaded:loadedItems_)images.push_back(loaded.image);
            CombineOptions o;
            o.layout=static_cast<CombineLayout>(combineLayoutCombo_->currentIndex());
            o.gridColumns=combineColsSpin_->value();
            o.spacingPx=combineSpacingSpin_->value();
            o.marginPx=combineMarginSpin_->value();
            o.transparentBackground=combineTransparentBgCheck_->isChecked();
            o.backgroundColor=combineBgColor_;
            o.alignment=static_cast<CombineAlignment>(combineAlignCombo_->currentIndex());
            o.scalePolicy=static_cast<CombineScalePolicy>(combineScalePolicyCombo_->currentIndex());
            return CombineOp::combine(images,o);
        }
        if(id=="color") return ColorAdjustOp::apply(image,colorOptions(this));
        if(id=="compress"){
            CompressionOptions o;
            o.format=compFormatCombo_->currentText();
            o.quality=compQualitySpin_->value();
            o.lossless=compLosslessCheck_->isChecked();
            o.targetMaxBytes=quint64(compTargetKiBSpin_->value())*1024;
            auto result=CompressWebOp::compressInMemory(image,o,item.fileProps.fileSizeBytes);
            if(!result.isOk())return Result<QImage>::failure(result.error.code,result.error.message,result.error.detail);
            lastCompressionMetrics_=*result.value;
            if(activeToolId_=="compress"){
                exportBar_->setSelectedFormat(compFormatCombo_->currentText());
                exportBar_->setQuality(result.value->chosenQuality);
            }
            compMetricsLabel_->setText(QString("Original: %1 bytes\nComprimido: %2 bytes\nAhorro: %3 %\nCalidad elegida: %4").arg(result.value->originalBytes).arg(result.value->compressedBytes).arg(result.value->savingsPercent,0,'f',1).arg(result.value->chosenQuality));
            QImage decoded=decodePreviewBytes(result.value->encodedBytes);
            if(decoded.isNull())return Result<QImage>::failure(ErrorCode::CorruptedInput,"No se pudo previsualizar el resultado comprimido");
            return Result<QImage>::success(decoded);
        }
        if(id=="web"){
            auto variants=CompressWebOp::generateWebVariants(image,QFileInfo(item.sourcePath).completeBaseName(),webOptions(this));
            if(!variants.isOk())return Result<QImage>::failure(variants.error.code,variants.error.message,variants.error.detail);
            QStringList lines;
            for(const auto& v:*variants.value)lines<<QString("%1 — %2 × %3 — %4 bytes").arg(v.fileName).arg(v.actualSize.width()).arg(v.actualSize.height()).arg(v.byteSize);
            webSummaryLabel_->setText(lines.join('\n'));
            if(variants.value->empty())return Result<QImage>::failure(ErrorCode::InvalidParameter,"Seleccione al menos una variante");
            return Result<QImage>::success(decodePreviewBytes(variants.value->front().encodedBytes));
        }
        if(id=="favicon"){
            auto bytes=IcoCodec::encodeMultiResolutionIco(image,{
                256
            },favTransparentCheck_->isChecked()?QColor(Qt::transparent):favBgColor_,static_cast<IconFitMode>(favFitModeCombo_->currentIndex()),favPaddingSpin_->value());
            if(!bytes.isOk())return Result<QImage>::failure(bytes.error.code,bytes.error.message,bytes.error.detail);
            QImage preview=QImage::fromData(*bytes.value,"ICO");
            if(preview.isNull())return Result<QImage>::failure(ErrorCode::CorruptedInput,"No se pudo previsualizar ICO");
            return Result<QImage>::success(preview);
        }
        if(id=="watermark"){
            WatermarkOptions o;
            o.useImageWatermark=wmUseImageCheck_->isChecked();
            o.text=wmTextEdit_->text();
            o.fontPointSize=wmFontSizeSpin_->value();
            o.textColor=wmTextColor_;
            o.watermarkImage=wmOverlayImage_;
            o.position=static_cast<AnchorPosition>(wmAnchorCombo_->currentIndex());
            o.opacity=wmOpacitySpin_->value();
            o.scalePercent=wmScaleSpin_->value();
            o.marginX=wmMarginSpin_->value();
            o.marginY=o.marginX;
            o.rotationDegrees=wmRotationSpin_->value();
            return WatermarkTextOp::applyWatermark(image,o);
        }
        if(id=="text_border"){
            TextAndBorderOptions o;
            o.text=tbTextEdit_->text();
            o.fontPointSize=tbFontSizeSpin_->value();
            o.textColor=tbTextColor_;
            o.textPosition=static_cast<AnchorPosition>(tbAnchorCombo_->currentIndex());
            o.paddingPx=tbPaddingSpin_->value();
            o.borderWidthPx=tbBorderWidthSpin_->value();
            o.borderColor=tbBorderColor_;
            o.cornerRadiusPx=tbCornerRadiusSpin_->value();
            return WatermarkTextOp::applyTextAndBorders(image,o);
        }
        if(id=="spritesheet"){
            std::vector<SpriteInput> sprites;
            for(const auto& loaded:loadedItems_)sprites.push_back({
                QFileInfo(loaded.sourcePath).fileName(),loaded.image
            });
            SpritesheetOptions o;
            o.columns=spriteColsSpin_->value();
            o.rows=spriteRowsSpin_->value();
            o.spacingPx=spriteSpacingSpin_->value();
            o.paddingPx=spritePaddingSpin_->value();
            auto result=SpritesheetOp::generate(sprites,o);
            if(!result.isOk())return Result<QImage>::failure(result.error.code,result.error.message,result.error.detail);
            lastSpritesheetResult_=*result.value;
            spriteJsonPreview_->setPlainText(QString::fromUtf8(result.value->coordinatesJson));
            return Result<QImage>::success(result.value->atlasImage);
        }
        if(id=="palette"){
            auto result=PaletteExtractorOp::extractDominantColors(image,palCountSpin_->value());
            if(!result.isOk())return Result<QImage>::failure(result.error.code,result.error.message,result.error.detail);
            currentPalette_=*result.value;
            std::vector<QStringList> rows;
            for(const auto& p:currentPalette_)rows.push_back({
                "",p.hex,QString("%1, %2, %3").arg(p.r).arg(p.g).arg(p.b),QString::number(p.dominanceRatio*100,'f',1)+" %"
            });
            fillTable(palTable_,rows);
            for(int row=0; row<int(currentPalette_.size()); ++row)palTable_->item(row,0)->setBackground(currentPalette_[size_t(row)].color);
            return PaletteExtractorOp::renderPaletteSwatch(currentPalette_);
        }
        if(id=="compare"){
            QImage other=loadedItems_.size()>1?loadedItems_[1].image:property("comparisonImage").value<QImage>();
            if(other.isNull())other=image;
            const QImage first=loadedItems_.size()>1?loadedItems_[0].image:image;
            auto result=ImageCompareOp::compare(first,other,static_cast<ComparisonMode>(cmpModeCombo_->currentIndex()),cmpSliderSpin_->value());
            if(!result.isOk())return Result<QImage>::failure(result.error.code,result.error.message,result.error.detail);
            cmpMetricsLabel_->setText(QString("MAE: %1\nRMSE: %2\nPSNR: %3 dB\nPíxeles diferentes: %4 %").arg(result.value->meanAbsoluteError,0,'f',3).arg(result.value->rmse,0,'f',3).arg(result.value->psnrDb,0,'f',2).arg(result.value->diffPixelPercent,0,'f',2));
            return Result<QImage>::success(result.value->renderedView);
        }
        if(id=="bg_remove"){
            BgRemovalOptions o;
            o.method=static_cast<BgRemovalMethod>(bgMethodCombo_->currentIndex());
            o.keyColor=bgKeyColor_;
            o.tolerance=bgToleranceSpin_->value();
            o.featherRadius=bgFeatherSpin_->value();
            o.replaceWithSolidColor=bgReplaceSolidCheck_->isChecked();
            o.replacementColor=bgReplacementColor_;
            return BackgroundRemovalOp::removeBackground(image,o);
        }
        if(id=="rename"){
            const auto preview=MassRenameOp::previewRename(loadedFilePaths(),renameOptions(this));
            std::vector<QStringList> rows;
            for(const auto& p:preview)rows.push_back({
                p.originalName,p.proposedName,p.hasConflict?"Conflicto de destino":"Disponible"
            });
            fillTable(renPreviewTable_,rows);
            return Result<QImage>::success(image);
        }
        if(id=="batch"){
            const BatchPreset p=batchPreset(this);
            QImage output=image;
            if(p.enableResize){
                auto result=ResizeOp::resize(output,p.resizeOptions);
                if(!result.isOk())return result;
                output=*result.value;
            }
            if(p.enableColorAdjust){
                auto result=ColorAdjustOp::apply(output,p.colorOptions);
                if(!result.isOk())return result;
                output=*result.value;
            }
            if(p.enableWatermark){
                auto result=WatermarkTextOp::applyWatermark(output,p.watermarkOptions);
                if(!result.isOk())return result;
                output=*result.value;
            }
            Q_UNUSED(itemIndex);
            return Result<QImage>::success(output);
        }
        if(id=="metadata"||id=="gif"||id=="pdf")return Result<QImage>::success(image);
        return Result<QImage>::failure(ErrorCode::InvalidParameter,"Herramienta no registrada",id);
    }
    void MainWindow::applyCurrentToolPreview() {
        if(auto* timer=findChild<QTimer*>("previewDebounce"))timer->stop();
        if(exportBar_->isBusy()||currentFileIndex_<0||currentFileIndex_>=int(loadedItems_.size()))return;
        auto result=evaluateToolOnImage(activeToolId_,loadedItems_[size_t(currentFileIndex_)],currentFileIndex_);
        if(!result.isOk()||result.value->isNull()){
            currentProcessedImage_={};
            exportBar_->setStatusSummary("No se pudo generar la vista previa: "+diagnostic(result.error));
            return;
        }
        currentProcessedImage_=std::move(*result.value);
        // Crop handles remain in source coordinates even after rotation or a non-full crop.
        canvas_->setPreviewImage(activeToolId_=="crop"?currentOriginalImage_:currentProcessedImage_);
        exportBar_->setStatusSummary(QString("Resultado preparado: %1 × %2 píxeles").arg(currentProcessedImage_.width()).arg(currentProcessedImage_.height()));
    }
    StatusResult MainWindow::exportSingleProcessedToFile(const QString& path,const SaveOptions& requested,bool overwrite) {
        if(!exportBar_->isBusy())if(auto* timer=findChild<QTimer*>("previewDebounce");timer && timer->isActive())applyCurrentToolPreview();
        if(exportBar_->isBusy())return StatusResult::failure(ErrorCode::ConflictDetected,"Hay una operación en curso");
        if(currentProcessedImage_.isNull()||currentFileIndex_<0)return emptyWorkspace();
        SaveOptions options=requested;
        if(options.format.isEmpty())options.format=QFileInfo(path).suffix();
        if(auto cap=FormatRegistry::findFormat(options.format))options.format=cap->id;
        if(activeToolId_=="convert")options.backgroundColor=convBgColor_;
        const auto& selected=loadedItems_[size_t(currentFileIndex_)];
        ImageMetadata meta=selected.metadata;
        meta.orientation=1;
        if(activeToolId_=="metadata"){
            auto edited=metadataForm(this,meta);
            if(!edited.isOk())return {
                edited.error
            };
            meta=*edited.value;
            meta.orientation=1;
        }
        if(activeToolId_=="convert"&&convColorSpaceCombo_->currentIndex()!=0){
            meta.iccProfileBytes=currentProcessedImage_.colorSpace().iccProfile();
            meta.colorProfileName=currentProcessedImage_.colorSpace().description();
            meta.hasIcc=!meta.iccProfileBytes.isEmpty();
        }
        activeCancelToken_.reset();
        StatusResult result;
        QStringList warnings;
        if(activeToolId_=="gif"&&options.format=="gif"){
            std::vector<GifFrame> frames;
            for(size_t i=0; i<loadedItems_.size(); ++i)frames.push_back({
                int(i)==currentFileIndex_?currentProcessedImage_:loadedItems_[i].image,gifDelaySpin_->value()
            });
            GifEncodeOptions gif;
            gif.defaultDelayMs=gifDelaySpin_->value();
            gif.loopCount=gifLoopSpin_->value();
            gif.dither=gifDitherCheck_->isChecked();
            result=GifCodec::saveAnimatedGif(frames,path,gif,overwrite);
        } else if(activeToolId_=="pdf"&&options.format=="pdf"){
            std::vector<QImage> pages;
            for(size_t i=0; i<loadedItems_.size(); ++i)pages.push_back(int(i)==currentFileIndex_?currentProcessedImage_:loadedItems_[i].image);
            PdfExportOptions pdf;
            pdf.pageSize=static_cast<PdfPageSizePreset>(pdfPageSizeCombo_->currentIndex());
            pdf.orientation=static_cast<PdfOrientation>(pdfOrientationCombo_->currentIndex());
            pdf.marginMm=pdfMarginSpin_->value();
            pdf.dpi=pdfDpiSpin_->value();
            pdf.proportionalFit=pdfProportionalCheck_->isChecked();
            result=PdfExporter::exportImagesToPdf(pages,path,pdf,overwrite,activeCancelToken_);
        } else if(activeToolId_=="favicon"&&options.format=="ico"){
            // The cached preview already contains fit and padding; encoding it again uses no padding.
            result=IcoCodec::saveMultiResolutionIco(currentProcessedImage_,path,{
                16,32,48,256
            },Qt::transparent,IconFitMode::ContainProportional,0,overwrite);
        } else if(activeToolId_=="compress"&&options.format==compFormatCombo_->currentText()&&!lastCompressionMetrics_.encodedBytes.isEmpty()){
            result=SafeFileWriter::writeWithCallback(path,[&](const QString& temp) {
                QFile out(temp);
                if(!out.open(QIODevice::WriteOnly|QIODevice::Truncate)||out.write(lastCompressionMetrics_.encodedBytes)!=lastCompressionMetrics_.encodedBytes.size())return StatusResult::failure(ErrorCode::FileWriteError,"No se pudo guardar el resultado comprimido");
                out.close();
                // Exact encoded bytes preserve the measured size; metadata was handled by the compressor.
                if(options.preserveMetadata)warnings<<"Se conserva el archivo comprimido medido; no se añaden metadatos que alteren el tamaño objetivo.";
                return StatusResult::success();
            },{
                overwrite,true
            });
        } else result=ImageIo::saveImage(currentProcessedImage_,path,options,&meta,overwrite,&warnings);
        if(result.isOk()){
            exportBar_->setLastExportedPath(path);
            ExportBarWidget::saveLastExportDirectory(QFileInfo(path).absolutePath());
            exportBar_->setStatusSummary(warnings.isEmpty()?"Archivo guardado. Puede guardar este mismo resultado de nuevo.":warnings.join(" | "));
        }
        return result;
    }
    StatusResult MainWindow::exportMultipleProcessedToFolder(const BatchFolderSaveRequest& request) {
        if(!exportBar_->isBusy())if(auto* timer=findChild<QTimer*>("previewDebounce");timer && timer->isActive())applyCurrentToolPreview();
        if(exportBar_->isBusy())return StatusResult::failure(ErrorCode::ConflictDetected,"Hay una operación en curso");
        if(loadedItems_.empty()||currentProcessedImage_.isNull())return emptyWorkspace();
        if(request.targetDirectory.isEmpty())return StatusResult::failure(ErrorCode::InvalidParameter,"Indique una carpeta de destino");
        QTemporaryDir stage;
        if(!stage.isValid())return StatusResult::failure(ErrorCode::FileWriteError,"No se pudo crear una carpeta temporal de exportación");
        SaveOptions options=request.saveOptions;
        if(options.format.isEmpty())options.format="png";
        if(auto cap=FormatRegistry::findFormat(options.format))options.format=cap->id;
        if(activeToolId_=="batch"){
            const auto preset=batchPreset(this);
            options.format=preset.outputFormat;
            options.quality=preset.quality;
            options.lossless=preset.lossless;
            options.preserveMetadata=!preset.stripMetadata;
        }
        const CompressionMetrics selectedCompression=lastCompressionMetrics_;
        const auto restoreCompression=qScopeGuard([this,selectedCompression]{
            lastCompressionMetrics_=selectedCompression;
        });
        activeCancelToken_.reset();
        QStringList outputNames;
        StatusResult status=StatusResult::success();
        if(activeToolId_=="batch"){
            const BatchPreset preset=batchPreset(this);
            const QStringList paths=loadedFilePaths();
            const auto token=activeCancelToken_;
            const QString directory=stage.path();
            exportBar_->setBusyState(true,0,int(paths.size()),"Procesando lote…");
            setProperty("folderBatchActive",true);
            auto* splitter=findChild<QSplitter*>("workspaceSplitter");
            splitter->setEnabled(false);
            topToolBar_->setEnabled(false);
            const auto restore=qScopeGuard([this,splitter]{
                setProperty("folderBatchActive",false);
                exportBar_->setBusyState(false);
                splitter->setEnabled(true);
                topToolBar_->setEnabled(true);
            });
            QPointer<MainWindow> guarded(this);
            const ProgressCallback progress=[guarded](int done,int total,const QString& path){
                if(!guarded)return;
                QMetaObject::invokeMethod(guarded,[guarded,done,total,path]{
                    if(guarded && guarded->property("folderBatchActive").toBool())guarded->exportBar_->setBusyState(true,done,total,"Procesado: "+QFileInfo(path).fileName());
                },Qt::QueuedConnection);
            };
            QEventLoop loop;
            QFutureWatcher<BatchSummary> watcher;
            connect(&watcher,&QFutureWatcher<BatchSummary>::finished,&loop,&QEventLoop::quit);
            watcher.setFuture(QtConcurrent::run([paths,directory,preset,token,progress]{
                return BatchProcessor::runBatch(paths,directory,preset,false,token,progress,0,saveBatchImage);
            }));
            loop.exec();
            BatchSummary summary;
            try{
                summary=watcher.result();
            } catch(const std::exception& error){
                return StatusResult::failure(ErrorCode::InternalError,"El lote falló",QString::fromUtf8(error.what()));
            }
            batchLogView_->clear();
            for(const auto& item:summary.itemResults){
                batchLogView_->appendPlainText(QFileInfo(item.sourcePath).fileName()+" — "+(item.success?QStringLiteral("Guardado"):"Error o cancelación: "+item.errorMessage));
                if(item.success)outputNames<<QFileInfo(item.outputPath).fileName();
            }
            batchLogView_->appendPlainText(QString("Total: %1; correctos: %2; errores: %3; cancelados: %4").arg(summary.totalItems).arg(summary.succeeded).arg(summary.failed).arg(summary.cancelled));
            if(summary.cancelled)return StatusResult::failure(ErrorCode::Cancelled,"Lote cancelado; no se publicaron archivos de salida");
            if(summary.failed)return StatusResult::failure(ErrorCode::FileWriteError,"El lote contiene errores; consulte el registro. No se publicaron archivos de salida");
        } else if(activeToolId_=="web"){
            const auto config=webOptions(this);
            for(const auto& item:loadedItems_){
                auto variants=CompressWebOp::generateWebVariants(item.image,QFileInfo(item.sourcePath).completeBaseName(),config);
                if(!variants.isOk())return {
                    variants.error
                };
                for(const auto& v:*variants.value){
                    if(!safeLeaf(v.fileName)||outputNames.contains(v.fileName,Qt::CaseInsensitive))return StatusResult::failure(ErrorCode::ConflictDetected,"Nombre de variante inseguro o duplicado",v.fileName);
                    status=SafeFileWriter::writeBytes(QDir(stage.path()).filePath(v.fileName),v.encodedBytes,{
                        false,true
                    });
                    if(!status.isOk())return status;
                    outputNames<<v.fileName;
                }
            }
        } else if(activeToolId_=="favicon"){
            FaviconExportOptions config;
            config.fitMode=static_cast<IconFitMode>(favFitModeCombo_->currentIndex());
            config.paddingPercent=favPaddingSpin_->value();
            config.backgroundColor=favTransparentCheck_->isChecked()?QColor(Qt::transparent):favBgColor_;
            config.generateWebManifest=favPwaManifestCheck_->isChecked();
            config.appName=favAppNameEdit_->text();
            config.appShortName=config.appName;
            auto result=IcoCodec::exportFaviconBundle(currentOriginalImage_,stage.path(),config,false);
            if(!result.isOk())return {
                result.error
            };
            outputNames<<QFileInfo(result.value->icoFilePath).fileName();
            for(const auto& path:result.value->pngFilePaths)outputNames<<QFileInfo(path).fileName();
            if(!result.value->manifestFilePath.isEmpty())outputNames<<QFileInfo(result.value->manifestFilePath).fileName();
        } else if(activeToolId_=="spritesheet"){
            if(lastSpritesheetResult_.atlasImage.isNull())return StatusResult::failure(ErrorCode::InvalidParameter,"Genere el atlas antes de exportar");
            outputNames={
                "atlas.png","atlas.json"
            };
            status=SpritesheetOp::saveSpritesheetAndJson(lastSpritesheetResult_,QDir(stage.path()).filePath(outputNames[0]),QDir(stage.path()).filePath(outputNames[1]),"png",false);
            if(!status.isOk())return status;
        } else if(activeToolId_=="palette"){
            const QStringList ext{
                "json","css","gpl","png"
            };
            const QString name="paleta."+ext[palExportFormatCombo_->currentIndex()];
            auto bytes=PaletteExtractorOp::exportPalette(currentPalette_,static_cast<PaletteExportFormat>(palExportFormatCombo_->currentIndex()),"Paleta Photo26");
            if(!bytes.isOk())return {
                bytes.error
            };
            status=SafeFileWriter::writeBytes(QDir(stage.path()).filePath(name),*bytes.value,{
                false,true
            });
            if(!status.isOk())return status;
            outputNames<<name;
        } else if(activeToolId_=="rename"){
            const auto preview=MassRenameOp::previewRename(loadedFilePaths(),renameOptions(this),stage.path());
            status=MassRenameOp::executeRenameOrCopy(preview,true,false);
            if(!status.isOk())return status;
            for(const auto& p:preview)outputNames<<p.proposedName;
        } else if(activeToolId_=="gif"||activeToolId_=="pdf"){
            options.format=activeToolId_;
            const QString name=activeToolId_=="gif"?"animacion.gif":"documento.pdf";
            const QString priorExport=exportBar_->lastExportedPath();
            const QString priorDirectory=ExportBarWidget::lastExportDirectory();
            status=exportSingleProcessedToFile(QDir(stage.path()).filePath(name),options,false);
            exportBar_->setLastExportedPath(priorExport);
            ExportBarWidget::saveLastExportDirectory(priorDirectory);
            if(!status.isOk())return status;
            outputNames<<name;
        } else{
            const bool aggregate=activeToolId_=="combine"||activeToolId_=="compare";
            QStringList sources=loadedFilePaths();
            if(aggregate)sources={
                sources[qMax(0,currentFileIndex_)]
            };
            QString pattern=request.namingPattern;
            if(activeToolId_=="batch")pattern=batchPreset(this).renameOptions.pattern+".{ext}";
            const auto preview=ExportBarWidget::buildBatchPreviewItems(sources,stage.path(),pattern,exportBar_->currentToolSuffix(),options.format);
            for(int i=0; i<int(preview.size()); ++i){
                if(activeCancelToken_.isCancelled())return StatusResult::failure(ErrorCode::Cancelled,"Exportación cancelada");
                const auto& p=preview[size_t(i)];
                const auto namedFormat=FormatRegistry::findFormat(QFileInfo(p.proposedFileName).suffix());
                if(!namedFormat || namedFormat->id!=options.format)return StatusResult::failure(ErrorCode::InvalidParameter,"La extensión del patrón debe coincidir con el formato de exportación",p.proposedFileName);
                if(!safeLeaf(p.proposedFileName)||outputNames.contains(p.proposedFileName,Qt::CaseInsensitive))return StatusResult::failure(ErrorCode::ConflictDetected,"Nombre de salida inseguro o duplicado",p.proposedFileName);
                const int index=aggregate?currentFileIndex_:i;
                const auto& item=loadedItems_[size_t(index)];
                if(activeToolId_=="compress" && options.format==compFormatCombo_->currentText()){
                    CompressionOptions config;
                    config.format=compFormatCombo_->currentText();
                    config.quality=compQualitySpin_->value();
                    config.lossless=compLosslessCheck_->isChecked();
                    config.targetMaxBytes=quint64(compTargetKiBSpin_->value())*1024;
                    auto compressed=CompressWebOp::compressInMemory(item.image,config,item.fileProps.fileSizeBytes);
                    if(!compressed.isOk())return {
                        compressed.error
                    };
                    status=SafeFileWriter::writeBytes(QDir(stage.path()).filePath(p.proposedFileName),compressed.value->encodedBytes,{
                        false,true
                    });
                    if(!status.isOk())return status;
                    outputNames<<p.proposedFileName;
                    continue;
                }
                auto result=index==currentFileIndex_?Result<QImage>::success(currentProcessedImage_):evaluateToolOnImage(activeToolId_,item,index);
                if(!result.isOk())return {
                    result.error
                };
                ImageMetadata meta=item.metadata;
                meta.orientation=1;
                if(activeToolId_=="metadata"){
                    auto edited=metadataForm(this,meta);
                    if(!edited.isOk())return {
                        edited.error
                    };
                    meta=*edited.value;
                    meta.orientation=1;
                }
                if(activeToolId_=="convert"&&convColorSpaceCombo_->currentIndex()!=0){
                    meta.iccProfileBytes=result.value->colorSpace().iccProfile();
                    meta.colorProfileName=result.value->colorSpace().description();
                }
                SaveOptions itemOptions=options;
                if(activeToolId_=="convert")itemOptions.backgroundColor=convBgColor_;
                if(activeToolId_=="batch"){
                    auto preset=batchPreset(this);
                    itemOptions.format=preset.outputFormat;
                    itemOptions.quality=preset.quality;
                    itemOptions.lossless=preset.lossless;
                    itemOptions.preserveMetadata=!preset.stripMetadata;
                }
                QStringList warnings;
                status=ImageIo::saveImage(*result.value,QDir(stage.path()).filePath(p.proposedFileName),itemOptions,&meta,false,&warnings);
                if(!status.isOk())return status;
                outputNames<<p.proposedFileName;
            }
        }
        if(outputNames.isEmpty())return StatusResult::failure(ErrorCode::InvalidParameter,"No se generó ningún archivo");
        const QDir destination(request.targetDirectory);
        QStringList sourceKeys;
        for(const auto& path:loadedFilePaths())sourceKeys<<QDir::cleanPath(QFileInfo(path).absoluteFilePath()).toCaseFolded();
        for(const auto& name:outputNames){
            const QString path=destination.absoluteFilePath(name);
            if(sourceKeys.contains(QDir::cleanPath(path).toCaseFolded()))return StatusResult::failure(ErrorCode::ConflictDetected,"La exportación no puede sobrescribir una imagen de origen",path);
            if(!request.allowOverwrite&&QFileInfo::exists(path))return StatusResult::failure(ErrorCode::ConflictDetected,"Ya existe un archivo de destino",path);
        }
        for(const auto& name:outputNames){
            const QString staged=QDir(stage.path()).filePath(name);
            status=SafeFileWriter::writeWithCallback(destination.filePath(name),[&](const QString& temp){
                QFile in(staged),out(temp);
                if(!in.open(QIODevice::ReadOnly)||!out.open(QIODevice::WriteOnly|QIODevice::Truncate))return StatusResult::failure(ErrorCode::FileWriteError,"No se pudo abrir un archivo de exportación");
                while(!in.atEnd()){
                    const auto bytes=in.read(1024*1024);
                    if(bytes.isEmpty()||out.write(bytes)!=bytes.size())return StatusResult::failure(ErrorCode::FileWriteError,"Error al copiar la exportación");
                }
                if(!out.flush())return StatusResult::failure(ErrorCode::FileWriteError,"Error finalizando la exportación");
                return StatusResult::success();
            },{
                request.allowOverwrite,true
            });
            if(!status.isOk())return status;
        }
        exportBar_->setLastExportedPath(request.targetDirectory);
        ExportBarWidget::saveLastExportDirectory(request.targetDirectory);
        exportBar_->setStatusSummary(QString("Guardados %1 archivos").arg(outputNames.size()));
        if(request.openFolderAfterSave)QDesktopServices::openUrl(QUrl::fromLocalFile(request.targetDirectory));
        return StatusResult::success();
    }
    StatusResult MainWindow::exportMultipleProcessedToZip(const QString& path,const SaveOptions& options,bool overwrite) {
        if(exportBar_->isBusy())return StatusResult::failure(ErrorCode::ConflictDetected,"Hay una operación en curso");
        if(loadedItems_.empty())return emptyWorkspace();
        if(!overwrite&&QFileInfo::exists(path))return StatusResult::failure(ErrorCode::ConflictDetected,"El archivo ZIP ya existe",path);
        QTemporaryDir stage;
        if(!stage.isValid())return StatusResult::failure(ErrorCode::FileWriteError,"No se pudo preparar el paquete ZIP");
        const QString priorDirectory=ExportBarWidget::lastExportDirectory(),priorExport=exportBar_->lastExportedPath();
        BatchFolderSaveRequest request;
        request.targetDirectory=stage.path();
        request.saveOptions=options;
        request.allowOverwrite=false;
        auto result=exportMultipleProcessedToFolder(request);
        ExportBarWidget::saveLastExportDirectory(priorDirectory);
        exportBar_->setLastExportedPath(priorExport);
        if(!result.isOk())return result;
        std::vector<std::pair<QString,QString>> files;
        const auto entries=QDir(stage.path()).entryInfoList(QDir::Files,QDir::Name);
        for(const auto& entry:entries)files.emplace_back(entry.absoluteFilePath(),entry.fileName());
        result=ZipWriter::createZipFromFiles(path,files,overwrite,activeCancelToken_);
        if(result.isOk()){
            exportBar_->setLastExportedPath(path);
            ExportBarWidget::saveLastExportDirectory(QFileInfo(path).absolutePath());
            exportBar_->setStatusSummary(QString("ZIP guardado con %1 archivos").arg(files.size()));
        }
        return result;
    }
    void MainWindow::setupKeyboardShortcuts() {
        homeAction_->setShortcut(QKeySequence("Alt+Home"));
        auto shortcut=[this](const QKeySequence& key,const std::function<void()>& action){
            auto* s=new QShortcut(key,this);
            connect(s,&QShortcut::activated,this,action);
        };
        shortcut(QKeySequence::Open,[this]{
            promptOpenFilesDialog();
        });
        shortcut(QKeySequence::SaveAs,[this] {
            if(currentFileIndex_<0||exportBar_->isBusy())return;
            QString path=exportBar_->promptSingleSaveFilePath(loadedItems_[size_t(currentFileIndex_)].sourcePath);
            if(!path.isEmpty())report(this,exportSingleProcessedToFile(path,exportBar_->currentSaveOptions()));
        });
        shortcut(QKeySequence("Ctrl+Return"),[this]{
            applyCurrentToolPreview();
        });
        shortcut(QKeySequence("Ctrl++"),[this]{
            canvas_->zoomIn();
        });
        shortcut(QKeySequence("Ctrl+-"),[this]{
            canvas_->zoomOut();
        });
        shortcut(QKeySequence("Ctrl+0"),[this]{
            canvas_->fitToWindow();
        });
        shortcut(QKeySequence(Qt::Key_Escape),[this]{
            if(exportBar_->isBusy())activeCancelToken_.cancel();
            else showHomeScreen();
        });
    }
    void MainWindow::restoreWindowPreferences() {
        QSettings settings;
        resize(1280,840);
        const auto geometry=settings.value("mainWindow/geometry").toByteArray();
        if(!geometry.isEmpty())restoreGeometry(geometry);
        const auto state=settings.value("mainWindow/state").toByteArray();
        if(!state.isEmpty())restoreState(state);
        if(auto* split=findChild<QSplitter*>("workspaceSplitter")){
            const auto saved=settings.value("mainWindow/splitter").toByteArray();
            if(!saved.isEmpty())split->restoreState(saved);
        }
    }
    void MainWindow::saveWindowPreferences() const {
        QSettings settings;
        settings.setValue("mainWindow/geometry",saveGeometry());
        settings.setValue("mainWindow/state",saveState());
        if(auto* split=findChild<QSplitter*>("workspaceSplitter"))settings.setValue("mainWindow/splitter",split->saveState());
        ThemeManager::saveTheme(currentTheme_);
    }
    void MainWindow::toggleTheme() {
        currentTheme_=currentTheme_==AppTheme::DarkModern?AppTheme::LightClean:AppTheme::DarkModern;
        ThemeManager::applyTheme(qobject_cast<QApplication*>(QCoreApplication::instance()),currentTheme_);
        ThemeManager::saveTheme(currentTheme_);
        homeGrid_->refreshIconsForTheme(currentTheme_);
        if(themeToggleAction_)themeToggleAction_->setText(currentTheme_==AppTheme::DarkModern?"Modo claro":"Modo oscuro");
        if(canvas_)canvas_->update();
    }
    void MainWindow::promptOpenFilesDialog() {
        if(exportBar_->isBusy())return;
        QStringList paths=QFileDialog::getOpenFileNames(this,"Abrir imágenes",ExportBarWidget::lastExportDirectory(),FormatRegistry::fileDialogReadFilter());
        if(!paths.isEmpty()){
            ExportBarWidget::saveLastExportDirectory(QFileInfo(paths.first()).absolutePath());
            loadFilesIntoWorkspace(paths);
        }
    }
    void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
        if(!exportBar_->isBusy()&&!droppedFiles(event->mimeData()).isEmpty())event->acceptProposedAction();
        else event->ignore();
    }
    void MainWindow::dropEvent(QDropEvent* event) {
        const auto files=droppedFiles(event->mimeData());
        if(!exportBar_->isBusy()&&!files.isEmpty()){
            loadFilesIntoWorkspace(files);
            event->acceptProposedAction();
        } else event->ignore();
    }
    void MainWindow::closeEvent(QCloseEvent* event) {
        if(exportBar_->isBusy()){
            activeCancelToken_.cancel();
            statusBar()->showMessage("Cancelando el lote; espere a que termine antes de cerrar",5000);
            event->ignore();
            return;
        }
        saveWindowPreferences();
        QMainWindow::closeEvent(event);
    }
    bool MainWindow::runSelfTestRoutine(QString* outDiagnosticReport) {
        if(exportBar_->isBusy()){
            if(outDiagnosticReport)*outDiagnosticReport="ERROR — El autodiagnóstico requiere que termine la operación activa";
            return false;
        }
        QStringList lines;
        bool ok=true;
        auto check=[&](bool success,const QString& message){
            lines<<(success?"CORRECTO — ":"ERROR — ")+message;
            ok=ok&&success;
        };
        check(rootStack_&&homeGrid_&&canvas_&&exportBar_&&paramStack_,"Creación de la ventana y widgets principales");
        QTemporaryDir directory;
        if(!directory.isValid()){
            if(outDiagnosticReport)*outDiagnosticReport="ERROR — No se pudo crear el directorio temporal";
            return false;
        }
        const QString oldTool=activeToolId_;
        const bool oldHome=isOnHomeScreen();
        const auto oldItems=loadedItems_;
        const int oldIndex=currentFileIndex_;
        const QString lastDirectory=ExportBarWidget::lastExportDirectory(),lastExport=exportBar_->lastExportedPath();
        QImage synthetic(96,64,QImage::Format_ARGB32);
        synthetic.fill(Qt::white);
        synthetic.setColorSpace(QColorSpace(QColorSpace::SRgb));
        {
            QPainter p(&synthetic);
            p.fillRect(8,8,32,40,QColor("#2679da"));
            p.fillRect(48,16,36,32,QColor("#e55d45"));
        }
        const QString source=QDir(directory.path()).filePath("sintetica.png");
        SaveOptions png;
        png.format="png";
        png.preserveMetadata=false;
        check(ImageIo::saveImage(synthetic,source,png,nullptr,false).isOk(),"Creación de imagen sintética PNG");
        const auto tools=ToolCardGridWidget::allTools();
        check(tools.size()==19,"Registro de 19 herramientas");
        for(const auto& tool:tools){
            clearWorkspaceFiles();
            activateTool(tool.id);
            check(activeToolId_==canonicalTool(tool.id)&&!isOnHomeScreen(),"Activación de "+tool.title);
            check(toolIdToParamPageIndex_.contains(tool.id),"Página de parámetros de "+tool.title);
            const bool loaded=loadFilesIntoWorkspace({
                source
            });
            check(loaded,"Carga de imagen en "+tool.title);
            if(!loaded)continue;
            applyCurrentToolPreview();
            if(tool.id=="crop"){
                check(currentProcessedImage_.size()==synthetic.size(),"El recorte inicial conserva toda la imagen");
                const int previousRotation=cropRotateQuadrants_;
                cropRotateQuadrants_=90;
                applyCurrentToolPreview();
                check(currentProcessedImage_.size()==QSize(synthetic.height(),synthetic.width()),"Rotación de recorte de 90 grados");
                cropRotateQuadrants_=previousRotation;
                applyCurrentToolPreview();
            }
            check(!currentProcessedImage_.isNull(),"Vista previa de "+tool.title);
            if(currentProcessedImage_.isNull())continue;
            SaveOptions save=png;
            save.format=tool.id=="gif"?"gif":tool.id=="pdf"?"pdf":tool.id=="favicon"?"ico":"png";
            QString target=QDir(directory.path()).filePath("prueba_"+tool.id+'.'+save.format);
            const QImage cached=currentProcessedImage_;
            auto result=exportSingleProcessedToFile(target,save,false);
            check(result.isOk()&&QFileInfo(target).size()>0,"Exportación de "+tool.title+(result.isOk()?QString():" — "+diagnostic(result.error)));
            if(result.isOk()){
                auto repeated=exportSingleProcessedToFile(QDir(directory.path()).filePath("repetida_"+tool.id+'.'+save.format),save,false);
                check(repeated.isOk()&&currentProcessedImage_==cached,"Guardado repetido sin reprocesar: "+tool.title);
                if(save.format=="png"){
                    auto read=ImageIo::loadImage(target);
                    check(read.isOk()&&read.value->image.size()==cached.size(),"Relectura PNG de "+tool.title);
                }
            }
        }
        clearWorkspaceFiles();
        loadedItems_=oldItems;
        currentFileIndex_=oldIndex;
        refreshFileListWidget();
        activateTool(oldTool);
        if(oldIndex>=0)loadSelectedFileIntoCanvas();
        if(oldHome)showHomeScreen();
        exportBar_->setLastExportedPath(lastExport);
        ExportBarWidget::saveLastExportDirectory(lastDirectory);
        lines<<QString("Resultado: %1; herramientas recorridas: %2").arg(ok?"CORRECTO":"FALLO").arg(tools.size());
        if(outDiagnosticReport)*outDiagnosticReport=lines.join('\n');
        return ok;
    }
}
// namespace photo26::desktop
