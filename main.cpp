#include "tcpapp.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    TcpApp w;
    w.show();

    return a.exec();
}
