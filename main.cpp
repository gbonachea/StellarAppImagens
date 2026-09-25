#include <QApplication>
#include <QIcon>
#include <QPalette>
#include <QStyleFactory>
#include "mainwindow.h"

static void applyDarkTheme(QApplication &app)
{
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette dark;
    dark.setColor(QPalette::Window,          QColor(30, 30, 30));
    dark.setColor(QPalette::WindowText,      QColor(212, 212, 212));
    dark.setColor(QPalette::Base,            QColor(40, 40, 40));
    dark.setColor(QPalette::AlternateBase,   QColor(50, 50, 50));
    dark.setColor(QPalette::ToolTipBase,     QColor(50, 50, 50));
    dark.setColor(QPalette::ToolTipText,     QColor(212, 212, 212));
    dark.setColor(QPalette::Text,            QColor(212, 212, 212));
    dark.setColor(QPalette::Button,          QColor(50, 50, 50));
    dark.setColor(QPalette::ButtonText,      QColor(212, 212, 212));
    dark.setColor(QPalette::BrightText,      QColor(255, 50, 50));
    dark.setColor(QPalette::Link,            QColor(100, 160, 255));
    dark.setColor(QPalette::Highlight,       QColor(42, 130, 218));
    dark.setColor(QPalette::HighlightedText, QColor(240, 240, 240));
    dark.setColor(QPalette::Disabled, QPalette::Text,       QColor(120, 120, 120));
    dark.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 120, 120));
    dark.setColor(QPalette::Disabled, QPalette::Window,     QColor(35, 35, 35));
    dark.setColor(QPalette::Disabled, QPalette::WindowText, QColor(90, 90, 90));

    app.setPalette(dark);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    applyDarkTheme(app);

    app.setApplicationName("Stellar AppImagens");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Stellar AppImagens");

    QIcon appIcon;
    appIcon.addFile(":/icons/stellarappImagens_16.png", QSize(16, 16));
    appIcon.addFile(":/icons/stellarappImagens_32.png", QSize(32, 32));
    appIcon.addFile(":/icons/stellarappImagens_48.png", QSize(48, 48));
    appIcon.addFile(":/icons/stellarappImagens_64.png", QSize(64, 64));
    appIcon.addFile(":/icons/stellarappImagens_128.png", QSize(128, 128));
    appIcon.addFile(":/icons/stellarappImagens_256.png", QSize(256, 256));
    appIcon.addFile(":/icons/stellarappImagens_512.png", QSize(512, 512));
    app.setWindowIcon(appIcon);

    MainWindow window;
    window.setWindowIcon(appIcon);
    window.show();

    return app.exec();
}
