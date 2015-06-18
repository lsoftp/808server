#include "mainwindow.h"
#include <QApplication>
#include "tcpserver.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qRegisterMetaType<string>("string");
    MainWindow w;
    w.show();
    TcpServer tser;
    tser.mw = &w;
    QObject::connect(&tser,SIGNAL(addPacket(string,string,string)),&w,SLOT(addrow(string,string,string)));
    tser.init();

    //tser.acceptsocket();
    return a.exec();
}
