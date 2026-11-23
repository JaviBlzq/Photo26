#include "theme_manager.hpp"

#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPen>
#include <QPolygonF>
#include <QRectF>
#include <QSettings>
#include <QStyleFactory>
#include <QtMath>
#include <algorithm>

namespace photo26::desktop {

namespace {

constexpr const char* kSettingsOrg = "Photo26";
constexpr const char* kSettingsApp = "Photo26Desktop";
constexpr const char* kThemeKey = "ui/theme_v2";

void drawSubtleBadgeBackground(QPainter& p, const QRectF& bounds, const QColor& accent) {
    QColor fill = accent;
    fill.setAlpha(24);
    QColor border = accent;
    border.setAlpha(58);

    p.setPen(QPen(border, 1.1));
    p.setBrush(fill);
    p.drawRoundedRect(bounds.adjusted(1.5, 1.5, -1.5, -1.5), 8.0, 8.0);
}

} // namespace

void ThemeManager::applyTheme(QApplication* app, AppTheme theme) {
    if (!app) {
        return;
    }

    if (QStyle* fusion = QStyleFactory::create(QStringLiteral("Fusion"))) {
        app->setStyle(fusion);
    }

    QPalette palette;
    if (theme == AppTheme::DarkModern) {
        const QColor windowBg(30, 32, 35);
        const QColor surfaceBg(38, 41, 46);
        const QColor altBg(45, 49, 55);
        const QColor textPrimary(230, 232, 236);
        const QColor textMuted(154, 161, 171);
        const QColor accent(74, 127, 168);

        palette.setColor(QPalette::Window, windowBg);
        palette.setColor(QPalette::WindowText, textPrimary);
        palette.setColor(QPalette::Base, QColor(27, 29, 32));
        palette.setColor(QPalette::AlternateBase, altBg);
        palette.setColor(QPalette::ToolTipBase, surfaceBg);
        palette.setColor(QPalette::ToolTipText, textPrimary);
        palette.setColor(QPalette::Text, textPrimary);
        palette.setColor(QPalette::PlaceholderText, textMuted);
        palette.setColor(QPalette::Button, surfaceBg);
        palette.setColor(QPalette::ButtonText, textPrimary);
        palette.setColor(QPalette::BrightText, Qt::white);
        palette.setColor(QPalette::Link, accent.lighter(120));
        palette.setColor(QPalette::Highlight, accent);
        palette.setColor(QPalette::HighlightedText, Qt::white);
        palette.setColor(QPalette::Disabled, QPalette::Text, textMuted.darker(125));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, textMuted.darker(125));
    } else {
        const QColor windowBg(242, 243, 245);
        const QColor surfaceBg(255, 255, 255);
        const QColor altBg(246, 247, 249);
        const QColor textPrimary(30, 35, 41);
        const QColor textMuted(95, 103, 115);
        const QColor accent(45, 95, 138);

        palette.setColor(QPalette::Window, windowBg);
        palette.setColor(QPalette::WindowText, textPrimary);
        palette.setColor(QPalette::Base, surfaceBg);
        palette.setColor(QPalette::AlternateBase, altBg);
        palette.setColor(QPalette::ToolTipBase, surfaceBg);
        palette.setColor(QPalette::ToolTipText, textPrimary);
        palette.setColor(QPalette::Text, textPrimary);
        palette.setColor(QPalette::PlaceholderText, textMuted);
        palette.setColor(QPalette::Button, surfaceBg);
        palette.setColor(QPalette::ButtonText, textPrimary);
        palette.setColor(QPalette::BrightText, Qt::white);
        palette.setColor(QPalette::Link, accent);
        palette.setColor(QPalette::Highlight, accent);
        palette.setColor(QPalette::HighlightedText, Qt::white);
        palette.setColor(QPalette::Disabled, QPalette::Text, textMuted);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, textMuted);
    }

    app->setPalette(palette);
    app->setStyleSheet(buildStyleSheet(theme));
}

QString ThemeManager::buildStyleSheet(AppTheme theme) {
    if (theme == AppTheme::DarkModern) {
        return QStringLiteral(R"(
            QMainWindow, QDialog {
                background-color: #1E2023;
                color: #E6E8EC;
            }
            QWidget {
                color: #E6E8EC;
                font-family: "Segoe UI", "Noto Sans", sans-serif;
                font-size: 13px;
            }
            QToolBar#navigationToolbar {
                background-color: #24272B;
                border: none;
                border-bottom: 1px solid #353A41;
                padding: 5px 12px;
                spacing: 6px;
            }
            QToolBar#navigationToolbar QToolButton {
                background-color: transparent;
                color: #DCE0E6;
                border: 1px solid transparent;
                border-radius: 5px;
                padding: 5px 10px;
                font-weight: 500;
            }
            QToolBar#navigationToolbar QToolButton:hover {
                background-color: #30343B;
                border-color: #434952;
            }
            QToolBar#navigationToolbar QToolButton:checked {
                background-color: #314454;
                border-color: #4A7FA8;
                color: #FFFFFF;
            }
            QLabel#ToolbarToolTitle {
                color: #E6E8EC;
                font-size: 13px;
                font-weight: 600;
                padding-left: 6px;
            }
            QLabel#MutedLabel {
                color: #9AA1AB;
            }
            QLabel#CategoryHeader {
                color: #CFD5DE;
                font-size: 13px;
                font-weight: 600;
                padding: 2px 0 4px 0;
                border-bottom: 1px solid #32373E;
            }
            QFrame#DropBanner {
                border: 1px dashed #4E5560;
                border-radius: 6px;
                background-color: #24272C;
                padding: 4px 10px;
            }
            QLabel#DropBannerLabel {
                border: none;
                background: transparent;
                color: #AEB5BF;
                font-size: 12px;
            }
            QWidget#ExportBar {
                background-color: #24272B;
                border-top: 1px solid #353A41;
            }
            QFrame#ToolCard, QPushButton#ToolCard {
                background-color: #26292E;
                border: 1px solid #363B43;
                border-radius: 7px;
                padding: 10px 12px;
                text-align: left;
            }
            QFrame#ToolCard:hover, QPushButton#ToolCard:hover {
                background-color: #2D3137;
                border: 1px solid #596574;
            }
            QFrame#ToolCard:pressed, QPushButton#ToolCard:pressed {
                background-color: #22252A;
            }
            QGroupBox {
                background-color: #25282D;
                border: 1px solid #363B43;
                border-radius: 6px;
                margin-top: 10px;
                padding: 12px 10px 10px 10px;
                font-weight: 600;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                subcontrol-position: top left;
                left: 10px;
                padding: 0 6px;
                color: #B4BCC6;
            }
            QPushButton {
                background-color: #2C3036;
                color: #E6E8EC;
                border: 1px solid #40464F;
                border-radius: 5px;
                padding: 6px 12px;
                font-weight: 500;
            }
            QPushButton:hover {
                background-color: #343940;
                border-color: #545C68;
            }
            QPushButton:pressed {
                background-color: #25282D;
            }
            QPushButton:disabled {
                background-color: #23262A;
                color: #6B727C;
                border-color: #30343B;
            }
            QPushButton#PrimaryButton {
                background-color: #3B6F99;
                border: 1px solid #4A7FA8;
                color: #FFFFFF;
                font-weight: 600;
            }
            QPushButton#PrimaryButton:hover {
                background-color: #467CA6;
                border-color: #5A90BA;
            }
            QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox, QPlainTextEdit, QTextEdit {
                background-color: #1B1D20;
                color: #E6E8EC;
                border: 1px solid #3B4048;
                border-radius: 5px;
                padding: 5px 8px;
                selection-background-color: #3B6F99;
            }
            QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus, QPlainTextEdit:focus, QTextEdit:focus {
                border: 1px solid #5A8BB3;
            }
            QListWidget, QTreeWidget, QTableWidget {
                background-color: #22252A;
                border: 1px solid #353A41;
                border-radius: 6px;
                outline: none;
            }
            QListWidget::item, QTreeWidget::item {
                padding: 6px 8px;
                border-radius: 4px;
            }
            QListWidget::item:selected, QTreeWidget::item:selected {
                background-color: #31485B;
                color: #FFFFFF;
            }
            QListWidget::item:hover, QTreeWidget::item:hover {
                background-color: #2B2F36;
            }
            QTabWidget::pane {
                border: 1px solid #353A41;
                border-radius: 5px;
                background-color: #24272C;
            }
            QTabBar::tab {
                background-color: #202327;
                color: #A2AAB5;
                border: 1px solid #353A41;
                border-bottom: none;
                border-top-left-radius: 5px;
                border-top-right-radius: 5px;
                padding: 6px 12px;
                margin-right: 2px;
            }
            QTabBar::tab:selected {
                background-color: #2B2F36;
                color: #FFFFFF;
                border-color: #4A7FA8;
            }
            QSlider::groove:horizontal {
                height: 5px;
                background: #363B43;
                border-radius: 2px;
            }
            QSlider::sub-page:horizontal {
                background: #4A7FA8;
                border-radius: 2px;
            }
            QSlider::handle:horizontal {
                background: #E6E8EC;
                border: 1px solid #4A7FA8;
                width: 13px;
                height: 13px;
                margin: -4px 0;
                border-radius: 7px;
            }
            QProgressBar {
                background-color: #1B1D20;
                border: 1px solid #363B43;
                border-radius: 5px;
                text-align: center;
                color: #E6E8EC;
                min-height: 18px;
            }
            QProgressBar::chunk {
                background-color: #3B6F99;
                border-radius: 4px;
            }
            QScrollBar:vertical {
                background: #1E2023;
                width: 10px;
                margin: 0px;
            }
            QScrollBar::handle:vertical {
                background: #3C414A;
                min-height: 24px;
                border-radius: 5px;
            }
            QScrollBar::handle:vertical:hover {
                background: #4F5662;
            }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
            QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
                height: 0px;
                width: 0px;
            }
            QStatusBar {
                background-color: #22252A;
                color: #9AA1AB;
                border-top: 1px solid #353A41;
            }
        )");
    }

    return QStringLiteral(R"(
        QMainWindow, QDialog {
            background-color: #F2F3F5;
            color: #1E2329;
        }
        QWidget {
            color: #1E2329;
            font-family: "Segoe UI", "Noto Sans", sans-serif;
            font-size: 13px;
        }
        QToolBar#navigationToolbar {
            background-color: #FFFFFF;
            border: none;
            border-bottom: 1px solid #DCDFE4;
            padding: 5px 12px;
            spacing: 6px;
        }
        QToolBar#navigationToolbar QToolButton {
            background-color: transparent;
            color: #2C333C;
            border: 1px solid transparent;
            border-radius: 5px;
            padding: 5px 10px;
            font-weight: 500;
        }
        QToolBar#navigationToolbar QToolButton:hover {
            background-color: #EEF1F4;
            border-color: #D4D8DF;
        }
        QToolBar#navigationToolbar QToolButton:checked {
            background-color: #E6EEF5;
            border-color: #8BA8C2;
            color: #1E4466;
        }
        QLabel#ToolbarToolTitle {
            color: #1E2329;
            font-size: 13px;
            font-weight: 600;
            padding-left: 6px;
        }
        QLabel#MutedLabel {
            color: #5F6773;
        }
        QLabel#CategoryHeader {
            color: #363E47;
            font-size: 13px;
            font-weight: 600;
            padding: 2px 0 4px 0;
            border-bottom: 1px solid #DCE0E6;
        }
        QFrame#DropBanner {
            border: 1px dashed #C3C9D2;
            border-radius: 6px;
            background-color: #FAFBFD;
            padding: 4px 10px;
        }
        QLabel#DropBannerLabel {
            border: none;
            background: transparent;
            color: #5F6773;
            font-size: 12px;
        }
        QWidget#ExportBar {
            background-color: #FFFFFF;
            border-top: 1px solid #DCDFE4;
        }
        QFrame#ToolCard, QPushButton#ToolCard {
            background-color: #FFFFFF;
            border: 1px solid #DCDFE5;
            border-radius: 7px;
            padding: 10px 12px;
            text-align: left;
        }
        QFrame#ToolCard:hover, QPushButton#ToolCard:hover {
            background-color: #F7F9FB;
            border: 1px solid #8DA4BA;
        }
        QFrame#ToolCard:pressed, QPushButton#ToolCard:pressed {
            background-color: #EEF2F6;
        }
        QGroupBox {
            background-color: #FFFFFF;
            border: 1px solid #DCDFE5;
            border-radius: 6px;
            margin-top: 10px;
            padding: 12px 10px 10px 10px;
            font-weight: 600;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 10px;
            padding: 0 6px;
            color: #424B56;
        }
        QPushButton {
            background-color: #FFFFFF;
            color: #242A32;
            border: 1px solid #C9CFD8;
            border-radius: 5px;
            padding: 6px 12px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #F3F5F7;
            border-color: #A7B0BC;
        }
        QPushButton:pressed {
            background-color: #E7EBEF;
        }
        QPushButton:disabled {
            background-color: #F4F5F7;
            color: #9AA2AE;
            border-color: #DFE3E8;
        }
        QPushButton#PrimaryButton {
            background-color: #2D5F8A;
            border: 1px solid #255075;
            color: #FFFFFF;
            font-weight: 600;
        }
        QPushButton#PrimaryButton:hover {
            background-color: #244F74;
        }
        QPushButton#PrimaryButton:pressed {
            background-color: #1D405E;
        }
        QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox, QPlainTextEdit, QTextEdit {
            background-color: #FFFFFF;
            color: #1E2329;
            border: 1px solid #C9CFD8;
            border-radius: 5px;
            padding: 5px 8px;
            selection-background-color: #2D5F8A;
            selection-color: #FFFFFF;
        }
        QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus, QPlainTextEdit:focus, QTextEdit:focus {
            border: 1px solid #2D5F8A;
        }
        QListWidget, QTreeWidget, QTableWidget {
            background-color: #FFFFFF;
            border: 1px solid #DCDFE5;
            border-radius: 6px;
            outline: none;
        }
        QListWidget::item, QTreeWidget::item {
            padding: 6px 8px;
            border-radius: 4px;
        }
        QListWidget::item:selected, QTreeWidget::item:selected {
            background-color: #E5EEF6;
            color: #1D405E;
        }
        QListWidget::item:hover, QTreeWidget::item:hover {
            background-color: #F3F5F8;
        }
        QTabWidget::pane {
            border: 1px solid #DCDFE5;
            border-radius: 5px;
            background-color: #FFFFFF;
        }
        QTabBar::tab {
            background-color: #ECEFF3;
            color: #4E5661;
            border: 1px solid #DCDFE5;
            border-bottom: none;
            border-top-left-radius: 5px;
            border-top-right-radius: 5px;
            padding: 6px 12px;
            margin-right: 2px;
        }
        QTabBar::tab:selected {
            background-color: #FFFFFF;
            color: #1E2329;
            border-color: #2D5F8A;
        }
        QSlider::groove:horizontal {
            height: 5px;
            background: #D0D5DD;
            border-radius: 2px;
        }
        QSlider::sub-page:horizontal {
            background: #2D5F8A;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background: #FFFFFF;
            border: 2px solid #2D5F8A;
            width: 13px;
            height: 13px;
            margin: -5px 0;
            border-radius: 7px;
        }
        QProgressBar {
            background-color: #E5E8ED;
            border: 1px solid #CFD4DC;
            border-radius: 5px;
            text-align: center;
            color: #1E2329;
            min-height: 18px;
        }
        QProgressBar::chunk {
            background-color: #2D5F8A;
            border-radius: 4px;
        }
        QScrollBar:vertical {
            background: #F2F3F5;
            width: 10px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #C5CBD4;
            min-height: 24px;
            border-radius: 5px;
        }
        QScrollBar::handle:vertical:hover {
            background: #A7B0BC;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            height: 0px;
            width: 0px;
        }
        QStatusBar {
            background-color: #FFFFFF;
            color: #5F6773;
            border-top: 1px solid #DCDFE4;
        }
    )");
}

AppTheme ThemeManager::loadSavedTheme() {
    QSettings settings(QString::fromLatin1(kSettingsOrg), QString::fromLatin1(kSettingsApp));
    const QString raw = settings.value(QString::fromLatin1(kThemeKey), QStringLiteral("light")).toString();
    if (raw.compare(QStringLiteral("dark"), Qt::CaseInsensitive) == 0) {
        return AppTheme::DarkModern;
    }
    return AppTheme::LightClean;
}

void ThemeManager::saveTheme(AppTheme theme) {
    QSettings settings(QString::fromLatin1(kSettingsOrg), QString::fromLatin1(kSettingsApp));
    settings.setValue(
        QString::fromLatin1(kThemeKey),
        theme == AppTheme::DarkModern ? QStringLiteral("dark") : QStringLiteral("light"));
    settings.sync();
}

QPixmap ThemeManager::createToolVectorPixmap(
    const QString& toolId,
    const QColor& accentColor,
    int sizePx,
    qreal devicePixelRatio) {
    const int logicalSize = std::max(16, sizePx);
    const qreal dpr = std::max<qreal>(1.0, devicePixelRatio);
    const int pixelSize = qRound(logicalSize * dpr);

    QPixmap pixmap(pixelSize, pixelSize);
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);

    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Coordinate space normalized to [0..40, 0..40]
    const qreal scale = static_cast<qreal>(logicalSize) / 40.0;
    p.scale(scale, scale);

    const QRectF canvas(0.0, 0.0, 40.0, 40.0);
    const QColor primary = accentColor.isValid() ? accentColor : QColor(45, 95, 138);
    const QColor detailTone = primary.darker(115);

    if (toolId == QStringLiteral("app_logo")) {
        // Clean, solid slate-blue desktop badge with white aperture emblem
        p.setPen(QPen(QColor(35, 76, 112), 1.2));
        p.setBrush(QColor(45, 95, 138));
        p.drawRoundedRect(QRectF(2.5, 2.5, 35.0, 35.0), 8.0, 8.0);

        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(Qt::white, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawEllipse(QPointF(20.0, 20.0), 10.2, 10.2);

        for (int i = 0; i < 6; ++i) {
            const qreal angle = qDegreesToRadians(static_cast<qreal>(i * 60));
            const QPointF p1(20.0 + 5.0 * qCos(angle), 20.0 + 5.0 * qSin(angle));
            const QPointF p2(20.0 + 10.0 * qCos(angle + 0.72), 20.0 + 10.0 * qSin(angle + 0.72));
            p.drawLine(p1, p2);
        }

        p.setBrush(QColor(255, 255, 255, 235));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(20.0, 20.0), 3.1, 3.1);
        return pixmap;
    }

    drawSubtleBadgeBackground(p, canvas, primary);

    QPen strokePen(primary, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QPen detailPen(detailTone, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(strokePen);
    p.setBrush(Qt::NoBrush);

    if (toolId == QStringLiteral("convert")) {
        p.drawRoundedRect(QRectF(8.0, 9.0, 13.0, 11.0), 2.2, 2.2);
        p.setPen(detailPen);
        p.drawRoundedRect(QRectF(19.0, 20.0, 13.0, 11.0), 2.2, 2.2);
        p.setPen(strokePen);
        QPainterPath arrows;
        arrows.moveTo(24.0, 11.0);
        arrows.lineTo(30.0, 11.0);
        arrows.lineTo(30.0, 16.5);
        arrows.moveTo(16.0, 29.0);
        arrows.lineTo(10.0, 29.0);
        arrows.lineTo(10.0, 23.5);
        p.drawPath(arrows);
    } else if (toolId == QStringLiteral("crop")) {
        QPainterPath cropPath;
        cropPath.moveTo(13.0, 8.0);
        cropPath.lineTo(13.0, 27.0);
        cropPath.lineTo(32.0, 27.0);
        cropPath.moveTo(8.0, 13.0);
        cropPath.lineTo(27.0, 13.0);
        cropPath.lineTo(27.0, 32.0);
        p.drawPath(cropPath);

        p.setPen(QPen(detailTone, 1.0, Qt::DashLine));
        p.drawLine(QPointF(13.0, 20.0), QPointF(27.0, 20.0));
        p.drawLine(QPointF(20.0, 13.0), QPointF(20.0, 27.0));
    } else if (toolId == QStringLiteral("resize")) {
        p.drawRoundedRect(QRectF(8.5, 8.5, 23.0, 23.0), 2.6, 2.6);
        p.setPen(detailPen);
        p.drawRoundedRect(QRectF(10.5, 18.5, 11.0, 11.0), 1.8, 1.8);
        p.setPen(strokePen);
        p.drawLine(QPointF(20.0, 20.0), QPointF(28.5, 11.5));
        p.drawLine(QPointF(23.5, 11.5), QPointF(28.5, 11.5));
        p.drawLine(QPointF(28.5, 11.5), QPointF(28.5, 16.5));
    } else if (toolId == QStringLiteral("combine")) {
        p.drawRoundedRect(QRectF(8.5, 9.0, 10.5, 22.0), 2.0, 2.0);
        p.setPen(detailPen);
        p.drawRoundedRect(QRectF(21.0, 9.0, 10.5, 10.0), 2.0, 2.0);
        p.drawRoundedRect(QRectF(21.0, 21.0, 10.5, 10.0), 2.0, 2.0);
    } else if (toolId == QStringLiteral("color")) {
        for (int i = 0; i < 3; ++i) {
            const qreal y = 12.0 + i * 8.0;
            p.setPen(strokePen);
            p.drawLine(QPointF(9.0, y), QPointF(31.0, y));
            const qreal knobX = (i == 0) ? 16.0 : (i == 1 ? 25.0 : 19.0);
            p.setBrush(primary);
            p.setPen(QPen(detailTone, 1.4));
            p.drawEllipse(QPointF(knobX, y), 2.8, 2.8);
        }
    } else if (toolId == QStringLiteral("compress")) {
        p.drawRoundedRect(QRectF(14.5, 14.5, 11.0, 11.0), 2.2, 2.2);
        p.setPen(detailPen);
        p.drawLine(QPointF(8.5, 8.5), QPointF(14.0, 14.0));
        p.drawLine(QPointF(31.5, 8.5), QPointF(26.0, 14.0));
        p.drawLine(QPointF(8.5, 31.5), QPointF(14.0, 26.0));
        p.drawLine(QPointF(31.5, 31.5), QPointF(26.0, 26.0));
    } else if (toolId == QStringLiteral("web")) {
        p.drawEllipse(QPointF(20.0, 20.0), 11.0, 11.0);
        p.setPen(detailPen);
        p.drawEllipse(QPointF(20.0, 20.0), 5.5, 11.0);
        p.drawLine(QPointF(9.5, 16.5), QPointF(30.5, 16.5));
        p.drawLine(QPointF(9.5, 23.5), QPointF(30.5, 23.5));
    } else if (toolId == QStringLiteral("favicon")) {
        p.drawRoundedRect(QRectF(8.5, 10.0, 23.0, 20.0), 2.6, 2.6);
        p.drawLine(QPointF(8.5, 15.5), QPointF(31.5, 15.5));
        p.setPen(detailPen);
        QPolygonF star;
        for (int i = 0; i < 10; ++i) {
            const qreal r = (i % 2 == 0) ? 5.0 : 2.2;
            const qreal a = qDegreesToRadians(static_cast<qreal>(i * 36 - 90));
            star << QPointF(20.0 + r * qCos(a), 22.8 + r * qSin(a));
        }
        p.setBrush(QColor(primary.red(), primary.green(), primary.blue(), 60));
        p.drawPolygon(star);
    } else if (toolId == QStringLiteral("metadata")) {
        QPainterPath tag;
        tag.moveTo(10.0, 10.0);
        tag.lineTo(22.0, 10.0);
        tag.lineTo(31.0, 19.0);
        tag.lineTo(21.0, 29.0);
        tag.lineTo(10.0, 18.0);
        tag.closeSubpath();
        p.drawPath(tag);
        p.setPen(detailPen);
        p.drawEllipse(QPointF(15.0, 15.0), 1.8, 1.8);
        p.drawLine(QPointF(17.5, 18.5), QPointF(24.0, 25.0));
    } else if (toolId == QStringLiteral("watermark")) {
        p.drawRoundedRect(QRectF(8.5, 10.0, 23.0, 20.0), 2.6, 2.6);
        p.setPen(detailPen);
        QPainterPath shield;
        shield.moveTo(20.0, 13.0);
        shield.lineTo(26.5, 15.5);
        shield.quadTo(26.5, 23.5, 20.0, 27.0);
        shield.quadTo(13.5, 23.5, 13.5, 15.5);
        shield.closeSubpath();
        p.drawPath(shield);
    } else if (toolId == QStringLiteral("text_border")) {
        p.drawRoundedRect(QRectF(8.5, 8.5, 23.0, 23.0), 2.6, 2.6);
        p.setPen(QPen(detailTone, 2.4, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(14.0, 14.5), QPointF(26.0, 14.5));
        p.drawLine(QPointF(20.0, 14.5), QPointF(20.0, 26.5));
    } else if (toolId == QStringLiteral("gif")) {
        p.drawRoundedRect(QRectF(8.5, 10.0, 23.0, 20.0), 2.5, 2.5);
        p.setPen(detailPen);
        QPolygonF play;
        play << QPointF(17.5, 15.5) << QPointF(25.0, 20.0) << QPointF(17.5, 24.5);
        p.setBrush(primary);
        p.drawPolygon(play);
    } else if (toolId == QStringLiteral("pdf")) {
        QPainterPath doc;
        doc.moveTo(11.0, 8.5);
        doc.lineTo(24.0, 8.5);
        doc.lineTo(29.5, 14.0);
        doc.lineTo(29.5, 31.5);
        doc.lineTo(11.0, 31.5);
        doc.closeSubpath();
        p.drawPath(doc);
        p.setPen(detailPen);
        p.drawLine(QPointF(24.0, 8.5), QPointF(24.0, 14.0));
        p.drawLine(QPointF(24.0, 14.0), QPointF(29.5, 14.0));
        p.drawLine(QPointF(15.0, 20.0), QPointF(25.5, 20.0));
        p.drawLine(QPointF(15.0, 24.5), QPointF(23.5, 24.5));
    } else if (toolId == QStringLiteral("spritesheet")) {
        p.drawRoundedRect(QRectF(9.0, 9.0, 22.0, 22.0), 2.2, 2.2);
        p.setPen(detailPen);
        p.drawLine(QPointF(16.3, 9.0), QPointF(16.3, 31.0));
        p.drawLine(QPointF(23.6, 9.0), QPointF(23.6, 31.0));
        p.drawLine(QPointF(9.0, 16.3), QPointF(31.0, 16.3));
        p.drawLine(QPointF(9.0, 23.6), QPointF(31.0, 23.6));
    } else if (toolId == QStringLiteral("palette")) {
        p.drawEllipse(QPointF(20.0, 20.0), 11.0, 11.0);
        const QList<QColor> dots = {
            QColor(175, 76, 76),
            QColor(186, 132, 62),
            QColor(68, 134, 106),
            QColor(62, 108, 150)};
        p.setPen(Qt::NoPen);
        for (int i = 0; i < dots.size(); ++i) {
            const qreal a = qDegreesToRadians(static_cast<qreal>(i * 75 - 135));
            p.setBrush(dots[i]);
            p.drawEllipse(QPointF(20.0 + 6.0 * qCos(a), 20.0 + 6.0 * qSin(a)), 2.3, 2.3);
        }
    } else if (toolId == QStringLiteral("compare")) {
        p.drawRoundedRect(QRectF(8.5, 10.0, 23.0, 20.0), 2.6, 2.6);
        p.setPen(QPen(detailTone, 2.0));
        p.drawLine(QPointF(20.0, 8.0), QPointF(20.0, 32.0));
        p.setBrush(primary);
        p.drawEllipse(QPointF(20.0, 20.0), 3.0, 3.0);
    } else if (toolId == QStringLiteral("bg_remove")) {
        p.drawRoundedRect(QRectF(8.5, 9.0, 23.0, 22.0), 2.6, 2.6);
        p.setPen(detailPen);
        p.drawEllipse(QPointF(17.0, 16.5), 3.1, 3.1);
        QPainterPath shoulders;
        shoulders.moveTo(11.5, 27.5);
        shoulders.quadTo(17.0, 21.0, 22.5, 27.5);
        p.drawPath(shoulders);
        p.drawLine(QPointF(26.5, 12.0), QPointF(26.5, 17.0));
        p.drawLine(QPointF(24.0, 14.5), QPointF(29.0, 14.5));
    } else if (toolId == QStringLiteral("rename")) {
        p.drawRoundedRect(QRectF(8.0, 13.5, 24.0, 13.0), 2.5, 2.5);
        p.setPen(detailPen);
        p.drawLine(QPointF(12.5, 20.0), QPointF(21.5, 20.0));
        p.setPen(QPen(detailTone, 2.0));
        p.drawLine(QPointF(25.5, 11.0), QPointF(25.5, 29.0));
        p.drawLine(QPointF(23.0, 11.0), QPointF(28.0, 11.0));
        p.drawLine(QPointF(23.0, 29.0), QPointF(28.0, 29.0));
    } else if (toolId == QStringLiteral("batch")) {
        p.setPen(detailPen);
        p.drawRoundedRect(QRectF(13.0, 8.5, 18.5, 14.5), 2.2, 2.2);
        p.setPen(strokePen);
        p.drawRoundedRect(QRectF(8.5, 15.5, 18.5, 15.5), 2.2, 2.2);
        QPainterPath bolt;
        bolt.moveTo(19.0, 18.5);
        bolt.lineTo(15.5, 23.5);
        bolt.lineTo(18.8, 23.5);
        bolt.lineTo(16.8, 28.0);
        p.setPen(QPen(detailTone, 1.9, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(bolt);
    } else {
        p.drawRoundedRect(QRectF(8.5, 10.0, 23.0, 20.0), 2.6, 2.6);
        p.setPen(detailPen);
        p.drawEllipse(QPointF(15.0, 15.5), 2.1, 2.1);
        QPainterPath mountains;
        mountains.moveTo(10.5, 27.5);
        mountains.lineTo(17.5, 20.5);
        mountains.lineTo(22.0, 25.0);
        mountains.lineTo(25.5, 21.5);
        mountains.lineTo(29.5, 27.5);
        p.drawPath(mountains);
    }

    return pixmap;
}

QIcon ThemeManager::createToolVectorIcon(
    const QString& toolId,
    const QColor& accentColor,
    int sizePx) {
    QIcon icon;
    icon.addPixmap(createToolVectorPixmap(toolId, accentColor, sizePx, 1.0));
    icon.addPixmap(createToolVectorPixmap(toolId, accentColor, sizePx, 2.0));
    return icon;
}

} // namespace photo26::desktop
