#include "AppController.h"
#include "MainWindow.h"

#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("PS5Streamer"));
    QApplication::setOrganizationName(QStringLiteral("PS5Streamer"));
    QApplication::setQuitOnLastWindowClosed(false);

    AppController controller;
    MainWindow window(&controller);
    window.show();

    return app.exec();
}
