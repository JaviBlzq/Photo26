#pragma once

#include <QMainWindow>

class QLabel;
class QPushButton;

class BootstrapWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit BootstrapWindow(QWidget* parent = nullptr);

    [[nodiscard]] int clickCount() const noexcept;
    [[nodiscard]] QPushButton* actionButton() const noexcept;
    [[nodiscard]] QLabel* statusLabel() const noexcept;

signals:
    void environmentChecked(int clickCount);

private slots:
    void verifyEnvironment();

private:
    int clickCount_{0};
    QLabel* statusLabel_{nullptr};
    QPushButton* actionButton_{nullptr};
};

