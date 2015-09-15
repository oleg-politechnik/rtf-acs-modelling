#include <QtGui/QApplication>
#include "mainwindow.h"

#include <QTextCodec>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName(QString::fromUtf8("Home work"));
    a.setOrganizationName(QString::fromUtf8("Oleg"));
    QTextCodec *utfcodec = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForTr(utfcodec);
    MainWindow w;
    w.show();
    return a.exec();
}
