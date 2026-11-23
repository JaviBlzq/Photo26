#include "bootstrap_window.hpp"

#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QtTest/QTest>

class BootstrapWindowTest final : public QObject
{
    Q_OBJECT

private slots:
    void titleAndInitialState();
    void buttonClickUpdatesState();
};

void BootstrapWindowTest::titleAndInitialState()
{
    BootstrapWindow window;
    QCOMPARE(window.windowTitle(), QStringLiteral("Photo26"));
    QCOMPARE(window.clickCount(), 0);
}

void BootstrapWindowTest::buttonClickUpdatesState()
{
    BootstrapWindow window;
    const QString initialText = window.statusLabel()->text();
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QTest::mouseClick(window.actionButton(), Qt::LeftButton);

    QCOMPARE(window.clickCount(), 1);
    QVERIFY(window.statusLabel()->text() != initialText);
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    BootstrapWindowTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_bootstrap_window.moc"
