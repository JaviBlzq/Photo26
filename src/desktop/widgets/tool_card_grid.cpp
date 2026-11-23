#include "tool_card_grid.hpp"

#include <QHBoxLayout>
#include <QMimeData>
#include <QUrl>
#include <algorithm>

namespace photo26::desktop {

namespace {

QStringList extractLocalPaths(const QMimeData* mimeData) {
    QStringList paths;
    if (!mimeData || !mimeData->hasUrls()) {
        return paths;
    }
    const QList<QUrl> urls = mimeData->urls();
    for (const QUrl& url : urls) {
        if (url.isLocalFile()) {
            paths.append(url.toLocalFile());
        }
    }
    return paths;
}

} // namespace

ToolCardButton::ToolCardButton(const ToolDescriptor& descriptor, QWidget* parent)
    : QPushButton(parent), descriptor_(descriptor) {
    setObjectName(QStringLiteral("ToolCard"));
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);
    setMinimumHeight(104);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(14, 12, 14, 12);
    rootLayout->setSpacing(14);

    iconLabel_ = new QLabel(this);
    iconLabel_->setFixedSize(44, 44);
    iconLabel_->setAlignment(Qt::AlignCenter);
    iconLabel_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    refreshIcon();

    auto* textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(4);

    titleLabel_ = new QLabel(descriptor_.title, this);
    QFont titleFont = titleLabel_->font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    titleLabel_->setFont(titleFont);
    titleLabel_->setWordWrap(true);
    titleLabel_->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    subtitleLabel_ = new QLabel(descriptor_.subtitle, this);
    subtitleLabel_->setObjectName(QStringLiteral("MutedLabel"));
    QFont subFont = subtitleLabel_->font();
    subFont.setPointSize(9);
    subtitleLabel_->setFont(subFont);
    subtitleLabel_->setWordWrap(true);
    subtitleLabel_->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    textLayout->addWidget(titleLabel_);
    textLayout->addWidget(subtitleLabel_);
    textLayout->addStretch(1);

    rootLayout->addWidget(iconLabel_, 0, Qt::AlignTop);
    rootLayout->addLayout(textLayout, 1);
}

void ToolCardButton::refreshIcon() {
    if (!iconLabel_) {
        return;
    }
    iconLabel_->setPixmap(
        ThemeManager::createToolVectorPixmap(descriptor_.id, descriptor_.accentColor, 42, 2.0));
}

void ToolCardButton::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        return;
    }
    QPushButton::dragEnterEvent(event);
}

void ToolCardButton::dropEvent(QDropEvent* event) {
    const QStringList paths = extractLocalPaths(event->mimeData());
    if (!paths.isEmpty()) {
        emit filesDroppedOnCard(paths, descriptor_.id);
        event->acceptProposedAction();
        return;
    }
    QPushButton::dropEvent(event);
}

// ============================================================================
// ToolCardGridWidget
// ============================================================================

ToolCardGridWidget::ToolCardGridWidget(QWidget* parent)
    : QWidget(parent) {
    setAcceptDrops(true);
    buildUi();
}

std::vector<ToolDescriptor> ToolCardGridWidget::allTools() {
    const QString catConv = QStringLiteral("Conversión y optimización");
    const QString catEdit = QStringLiteral("Edición y geometría");
    const QString catComp = QStringLiteral("Composición y documentos");
    const QString catUtil = QStringLiteral("Metadatos y utilidades");

    const QColor toneConv(53, 99, 138);   // Azul pizarra sobrio
    const QColor toneEdit(59, 117, 98);   // Verde salvia natural
    const QColor toneComp(143, 99, 56);   // Ocre tierra suave
    const QColor toneUtil(85, 98, 112);   // Grafito azulado neutro

    return {
        // Categoría 1: Conversión y optimización
        {
            QStringLiteral("convert"),
            QStringLiteral("Convertir formato"),
            QStringLiteral("Convierte entre PNG, JPEG, WebP, BMP, TIFF e ICO con control de calidad."),
            catConv,
            toneConv,
            QStringLiteral("convertida")
        },
        {
            QStringLiteral("compress"),
            QStringLiteral("Comprimir imagen"),
            QStringLiteral("Reduce el peso en disco ajustando calidad o tamaño objetivo en KB."),
            catConv,
            toneConv,
            QStringLiteral("comprimida")
        },
        {
            QStringLiteral("web"),
            QStringLiteral("Optimizar para Web"),
            QStringLiteral("Prepara imágenes ligeras en sRGB y dimensiones óptimas para sitios web."),
            catConv,
            toneConv,
            QStringLiteral("web")
        },
        {
            QStringLiteral("favicon"),
            QStringLiteral("Generador de Favicon e ICO"),
            QStringLiteral("Crea iconos multi-resolución (.ico) y paquetes de iconos de aplicación."),
            catConv,
            toneConv,
            QStringLiteral("favicon")
        },
        {
            QStringLiteral("batch"),
            QStringLiteral("Procesamiento por lotes"),
            QStringLiteral("Aplica conversión, escalado, marca de agua y optimización a múltiples archivos."),
            catConv,
            toneConv,
            QStringLiteral("lote")
        },

        // Categoría 2: Edición y geometría
        {
            QStringLiteral("crop"),
            QStringLiteral("Recortar y enderezar"),
            QStringLiteral("Recorte interactivo con proporciones fijas o libres y regla de los tercios."),
            catEdit,
            toneEdit,
            QStringLiteral("recortada")
        },
        {
            QStringLiteral("resize"),
            QStringLiteral("Redimensionar y rotar"),
            QStringLiteral("Cambia dimensiones en píxeles o porcentaje, voltea y rota con precisión."),
            catEdit,
            toneEdit,
            QStringLiteral("redimensionada")
        },
        {
            QStringLiteral("color"),
            QStringLiteral("Ajustes de color y filtros"),
            QStringLiteral("Brillo, contraste, saturación, exposición, gamma, blanco y negro y sepia."),
            catEdit,
            toneEdit,
            QStringLiteral("ajustada")
        },
        {
            QStringLiteral("bg_remove"),
            QStringLiteral("Eliminar fondo / Transparencia"),
            QStringLiteral("Extrae fondos uniformes o por color (Chroma Key) con suavizado de bordes."),
            catEdit,
            toneEdit,
            QStringLiteral("sin_fondo")
        },
        {
            QStringLiteral("watermark"),
            QStringLiteral("Marca de agua"),
            QStringLiteral("Superpón texto o logotipo con opacidad, rotación, posición y mosaico."),
            catEdit,
            toneEdit,
            QStringLiteral("marca_agua")
        },

        // Categoría 3: Composición y documentos
        {
            QStringLiteral("combine"),
            QStringLiteral("Combinar y collage"),
            QStringLiteral("Une varias imágenes en horizontal, vertical o cuadrícula con margen y fondo."),
            catComp,
            toneComp,
            QStringLiteral("combinada")
        },
        {
            QStringLiteral("text_border"),
            QStringLiteral("Texto, marcos y bordes"),
            QStringLiteral("Añade marcos decorativos, esquinas redondeadas, relleno y rótulos tipográficos."),
            catComp,
            toneComp,
            QStringLiteral("texto_borde")
        },
        {
            QStringLiteral("pdf"),
            QStringLiteral("Imágenes a PDF"),
            QStringLiteral("Genera documentos PDF multipágina limpios a partir de tus fotografías."),
            catComp,
            toneComp,
            QStringLiteral("documento")
        },
        {
            QStringLiteral("gif"),
            QStringLiteral("Animación GIF / Secuencia"),
            QStringLiteral("Crea secuencias animadas ajustando duración por fotograma y bucle."),
            catComp,
            toneComp,
            QStringLiteral("animacion")
        },
        {
            QStringLiteral("spritesheet"),
            QStringLiteral("Hoja de Sprites (Spritesheet)"),
            QStringLiteral("Empaqueta fotogramas en un atlas y exporta el mapa de coordenadas JSON."),
            catComp,
            toneComp,
            QStringLiteral("spritesheet")
        },

        // Categoría 4: Metadatos y utilidades
        {
            QStringLiteral("metadata"),
            QStringLiteral("Inspector y limpieza de metadatos"),
            QStringLiteral("Consulta, edita y elimina metadatos EXIF, XMP, IPTC e ICC para privacidad."),
            catUtil,
            toneUtil,
            QStringLiteral("metadatos")
        },
        {
            QStringLiteral("palette"),
            QStringLiteral("Extractor de paleta de colores"),
            QStringLiteral("Obtén los colores dominantes de la imagen con códigos HEX y RGB."),
            catUtil,
            toneUtil,
            QStringLiteral("paleta")
        },
        {
            QStringLiteral("compare"),
            QStringLiteral("Comparador visual de imágenes"),
            QStringLiteral("Inspecciona diferencias Antes / Después con cortina deslizante y mapa de diferencias."),
            catUtil,
            toneUtil,
            QStringLiteral("comparacion")
        },
        {
            QStringLiteral("rename"),
            QStringLiteral("Renombrado masivo inteligente"),
            QStringLiteral("Organiza colecciones con patrones de numeración, prefijos, sufijos y dimensiones."),
            catUtil,
            toneUtil,
            QStringLiteral("renombrada")
        }
    };
}

std::optional<ToolDescriptor> ToolCardGridWidget::findTool(const QString& toolId) {
    const auto tools = allTools();
    for (const auto& t : tools) {
        if (t.id == toolId) {
            return t;
        }
    }
    return std::nullopt;
}

void ToolCardGridWidget::buildUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 18, 24, 18);
    mainLayout->setSpacing(14);

    // 1. Hero Header Banner
    auto* heroRow = new QHBoxLayout();
    heroRow->setSpacing(16);

    appLogoLabel_ = new QLabel(this);
    appLogoLabel_->setFixedSize(44, 44);
    appLogoLabel_->setPixmap(
        ThemeManager::createToolVectorPixmap(QStringLiteral("app_logo"), QColor(45, 95, 138), 44, 2.0));

    auto* titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);
    auto* mainTitle = new QLabel(QStringLiteral("Photo26 — Herramientas de imagen"), this);
    QFont heroFont = mainTitle->font();
    heroFont.setPointSize(15);
    heroFont.setBold(true);
    mainTitle->setFont(heroFont);

    auto* subTitle = new QLabel(
        QStringLiteral("Selecciona una herramienta o arrastra imágenes para empezar (procesamiento 100 % local)"),
        this);
    subTitle->setObjectName(QStringLiteral("MutedLabel"));

    titleCol->addWidget(mainTitle);
    titleCol->addWidget(subTitle);

    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(QStringLiteral("Buscar herramienta (recortar, webp, pdf, fondo, lote)..."));
    searchEdit_->setClearButtonEnabled(true);
    searchEdit_->setMinimumWidth(290);
    searchEdit_->setMaximumWidth(380);

    connect(searchEdit_, &QLineEdit::textChanged, this, &ToolCardGridWidget::setSearchFilter);

    heroRow->addWidget(appLogoLabel_);
    heroRow->addLayout(titleCol, 1);
    heroRow->addWidget(searchEdit_, 0, Qt::AlignRight | Qt::AlignVCenter);
    mainLayout->addLayout(heroRow);

    // 2. Subtle Drag & Drop Banner
    dropBanner_ = new QFrame(this);
    dropBanner_->setObjectName(QStringLiteral("DropBanner"));
    auto* bannerLayout = new QHBoxLayout(dropBanner_);
    bannerLayout->setContentsMargins(10, 5, 10, 5);
    auto* bannerLabel = new QLabel(
        QStringLiteral("Arrastra imágenes aquí o directamente sobre cualquier tarjeta para abrir esa herramienta"),
        dropBanner_);
    bannerLabel->setObjectName(QStringLiteral("DropBannerLabel"));
    bannerLabel->setAlignment(Qt::AlignCenter);
    bannerLayout->addWidget(bannerLabel);
    mainLayout->addWidget(dropBanner_);

    // 3. Scrollable Categories Area
    scrollArea_ = new QScrollArea(this);
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setFrameShape(QFrame::NoFrame);

    scrollContent_ = new QWidget(scrollArea_);
    categoriesLayout_ = new QVBoxLayout(scrollContent_);
    categoriesLayout_->setContentsMargins(2, 4, 8, 16);
    categoriesLayout_->setSpacing(18);

    const auto tools = allTools();
    const QStringList categoryOrder = {
        QStringLiteral("Conversión y optimización"),
        QStringLiteral("Edición y geometría"),
        QStringLiteral("Composición y documentos"),
        QStringLiteral("Metadatos y utilidades")
    };

    for (const QString& catName : categoryOrder) {
        CategorySection sec;
        sec.categoryName = catName;
        sec.container = new QWidget(scrollContent_);
        auto* secLayout = new QVBoxLayout(sec.container);
        secLayout->setContentsMargins(0, 0, 0, 0);
        secLayout->setSpacing(10);

        sec.headerLabel = new QLabel(catName, sec.container);
        sec.headerLabel->setObjectName(QStringLiteral("CategoryHeader"));

        sec.gridLayout = new QGridLayout();
        sec.gridLayout->setContentsMargins(0, 0, 0, 0);
        sec.gridLayout->setHorizontalSpacing(12);
        sec.gridLayout->setVerticalSpacing(12);

        for (const auto& t : tools) {
            if (t.category != catName) {
                continue;
            }
            auto* card = new ToolCardButton(t, sec.container);
            connect(card, &QPushButton::clicked, this, [this, id = t.id]() {
                emit toolActivated(id);
            });
            connect(card, &ToolCardButton::filesDroppedOnCard, this, &ToolCardGridWidget::filesDroppedOnHome);
            sec.cards.push_back(card);
        }

        secLayout->addWidget(sec.headerLabel);
        secLayout->addLayout(sec.gridLayout);
        categoriesLayout_->addWidget(sec.container);
        sections_.push_back(std::move(sec));
    }

    emptyStateLabel_ = new QLabel(
        QStringLiteral("No se encontraron herramientas que coincidan con el filtro de búsqueda."),
        scrollContent_);
    emptyStateLabel_->setObjectName(QStringLiteral("MutedLabel"));
    emptyStateLabel_->setAlignment(Qt::AlignCenter);
    emptyStateLabel_->setVisible(false);
    categoriesLayout_->addWidget(emptyStateLabel_);
    categoriesLayout_->addStretch(1);

    scrollArea_->setWidget(scrollContent_);
    mainLayout->addWidget(scrollArea_, 1);

    currentColumnCount_ = computeColumnCount();
    rebuildCategoryLayout();
}

int ToolCardGridWidget::computeColumnCount() const {
    const int w = width();
    if (w >= 1340) return 4;
    if (w >= 920) return 3;
    if (w >= 580) return 2;
    return 1;
}

void ToolCardGridWidget::rebuildCategoryLayout() {
    const QString query = searchEdit_ ? searchEdit_->text().trimmed() : QString();
    const int cols = std::max(1, currentColumnCount_);
    int totalVisibleCards = 0;

    for (auto& sec : sections_) {
        while (QLayoutItem* item = sec.gridLayout->takeAt(0)) {
            delete item;
        }

        int visibleInSec = 0;
        for (ToolCardButton* card : sec.cards) {
            const auto& d = card->descriptor();
            const bool matches = query.isEmpty() ||
                                 d.title.contains(query, Qt::CaseInsensitive) ||
                                 d.subtitle.contains(query, Qt::CaseInsensitive) ||
                                 d.category.contains(query, Qt::CaseInsensitive) ||
                                 d.id.contains(query, Qt::CaseInsensitive);

            card->setVisible(matches);
            if (matches) {
                const int row = visibleInSec / cols;
                const int col = visibleInSec % cols;
                sec.gridLayout->addWidget(card, row, col);
                ++visibleInSec;
                ++totalVisibleCards;
            }
        }

        for (int c = 0; c < cols; ++c) {
            sec.gridLayout->setColumnStretch(c, 1);
        }

        sec.container->setVisible(visibleInSec > 0);
    }

    if (emptyStateLabel_) {
        emptyStateLabel_->setVisible(totalVisibleCards == 0);
    }
}

void ToolCardGridWidget::setSearchFilter(const QString& text) {
    if (searchEdit_ && searchEdit_->text() != text) {
        searchEdit_->setText(text);
        return;
    }
    rebuildCategoryLayout();
}

QString ToolCardGridWidget::searchFilter() const {
    return searchEdit_ ? searchEdit_->text() : QString();
}

void ToolCardGridWidget::refreshIconsForTheme(AppTheme theme) {
    currentTheme_ = theme;
    if (appLogoLabel_) {
        appLogoLabel_->setPixmap(
            ThemeManager::createToolVectorPixmap(QStringLiteral("app_logo"), QColor(45, 95, 138), 44, 2.0));
    }
    for (auto& sec : sections_) {
        for (ToolCardButton* card : sec.cards) {
            card->refreshIcon();
        }
    }
}

void ToolCardGridWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    const int newCols = computeColumnCount();
    if (newCols != currentColumnCount_) {
        currentColumnCount_ = newCols;
        rebuildCategoryLayout();
    }
}

void ToolCardGridWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void ToolCardGridWidget::dropEvent(QDropEvent* event) {
    const QStringList paths = extractLocalPaths(event->mimeData());
    if (!paths.isEmpty()) {
        emit filesDroppedOnHome(paths, QStringLiteral("convert"));
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

} // namespace photo26::desktop
