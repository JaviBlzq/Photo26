#pragma once

#include "theme_manager.hpp"

#include <QColor>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#include <optional>
#include <vector>

namespace photo26::desktop {

struct ToolDescriptor {
    QString id;
    QString title;
    QString subtitle;
    QString category;
    QColor accentColor;
    QString defaultSuffix;
};

class ToolCardButton final : public QPushButton {
    Q_OBJECT

public:
    explicit ToolCardButton(const ToolDescriptor& descriptor, QWidget* parent = nullptr);
    ~ToolCardButton() override = default;

    const ToolDescriptor& descriptor() const noexcept { return descriptor_; }
    void refreshIcon();

signals:
    void filesDroppedOnCard(const QStringList& filePaths, const QString& toolId);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    ToolDescriptor descriptor_;
    QLabel* iconLabel_ = nullptr;
    QLabel* titleLabel_ = nullptr;
    QLabel* subtitleLabel_ = nullptr;
};

class ToolCardGridWidget : public QWidget {
    Q_OBJECT

public:
    explicit ToolCardGridWidget(QWidget* parent = nullptr);
    ~ToolCardGridWidget() override = default;

    static std::vector<ToolDescriptor> allTools();
    static std::optional<ToolDescriptor> findTool(const QString& toolId);

    void setSearchFilter(const QString& text);
    QString searchFilter() const;
    void refreshIconsForTheme(AppTheme theme);

signals:
    void toolActivated(const QString& toolId);
    void filesDroppedOnHome(const QStringList& filePaths, const QString& targetToolId);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    struct CategorySection {
        QString categoryName;
        QWidget* container = nullptr;
        QLabel* headerLabel = nullptr;
        QGridLayout* gridLayout = nullptr;
        std::vector<ToolCardButton*> cards;
    };

    void buildUi();
    void rebuildCategoryLayout();
    int computeColumnCount() const;

    QLabel* appLogoLabel_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QFrame* dropBanner_ = nullptr;
    QScrollArea* scrollArea_ = nullptr;
    QWidget* scrollContent_ = nullptr;
    QVBoxLayout* categoriesLayout_ = nullptr;
    QLabel* emptyStateLabel_ = nullptr;

    std::vector<CategorySection> sections_;
    int currentColumnCount_ = 0;
    AppTheme currentTheme_ = AppTheme::LightClean;
};

} // namespace photo26::desktop
