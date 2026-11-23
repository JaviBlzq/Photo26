#include "bootstrap_window.hpp"

#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

namespace {
constexpr auto initialStatus = "Entorno pendiente de verificación";
constexpr auto windowWidth = 420;
constexpr auto windowHeight = 180;
}

BootstrapWindow::BootstrapWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Photo26"));
    resize(windowWidth, windowHeight);

    auto* centralWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(centralWidget);
    statusLabel_ = new QLabel(QString::fromUtf8(initialStatus), centralWidget);
    actionButton_ = new QPushButton(QStringLiteral("Verificar entorno"), centralWidget);

    statusLabel_->setObjectName(QStringLiteral("statusLabel"));
    actionButton_->setObjectName(QStringLiteral("actionButton"));
    layout->addWidget(statusLabel_);
    layout->addWidget(actionButton_);
    setCentralWidget(centralWidget);

    connect(actionButton_, &QPushButton::clicked,
            this, &BootstrapWindow::verifyEnvironment);
}

int BootstrapWindow::clickCount() const noexcept
{
    return clickCount_;
}

QPushButton* BootstrapWindow::actionButton() const noexcept
{
    return actionButton_;
}

QLabel* BootstrapWindow::statusLabel() const noexcept
{
    return statusLabel_;
}

void BootstrapWindow::verifyEnvironment()
{
    ++clickCount_;
    statusLabel_->setText(
        QStringLiteral("Entorno verificado (%1)").arg(clickCount_));
    emit environmentChecked(clickCount_);
}

