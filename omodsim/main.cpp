#include <QApplication>
#include <QFontDatabase>
#include "mainwindow.h"

///
/// \brief main
/// \param argc
/// \param argv
/// \return
///
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName(APP_NAME);
    a.setApplicationVersion(APP_VERSION);
    QFontDatabase::addApplicationFont(":/fonts/firacode.ttf");

    MainWindow w;
    w.show();

    return a.exec();
}
