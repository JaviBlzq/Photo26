#pragma once

#include <QApplication>
#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>

namespace photo26::desktop {

enum class AppTheme {
    DarkModern,
    LightClean
};

class ThemeManager {
public:
    static void applyTheme(QApplication* app, AppTheme theme);
    static QString buildStyleSheet(AppTheme theme);
    static AppTheme loadSavedTheme();
    static void saveTheme(AppTheme theme);

    static QIcon createToolVectorIcon(
        const QString& toolId,
        const QColor& accentColor,
        int sizePx = 40);

    static QPixmap createToolVectorPixmap(
        const QString& toolId,
        const QColor& accentColor,
        int sizePx = 40,
        qreal devicePixelRatio = 2.0);

    static bool isDarkTheme(AppTheme theme) noexcept {
        return theme == AppTheme::DarkModern;
    }
};

} // namespace photo26::desktop
