#if !defined(SMARTGLOVEPLUGINLIB)
#include <QApplication>
#include "mainwindow.h"
#else
#include <QCoreApplication>
#endif

int main(int argc, char *argv[])
{
#if !defined(SMARTGLOVEPLUGINLIB)
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
#else
    QCoreApplication a(argc, argv);
    return a.exec();
#endif
}
