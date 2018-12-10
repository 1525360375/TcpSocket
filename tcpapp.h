#ifndef TCPAPP_H
#define TCPAPP_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QFile>
#include <QTimer>
#include <QMessageBox>
#include <QDebug>
#include <QListWidgetItem>
#include <QFileDialog>

namespace Ui {
class TcpApp;
}

class TcpApp : public QMainWindow
{
    Q_OBJECT

public:
    explicit TcpApp(QWidget *parent = 0);
    ~TcpApp();

private slots:
    void on_serverRB_clicked();//选择作为服务器
    void on_clientRB_clicked();//选择作为客户端
    void on_StartBt_clicked();//启动服务器或链接客户端
    void on_closeBt_clicked();//关闭服务器或断开客户端
    void on_onlineUserList_doubleClicked(const QModelIndex &index);//选择给哪个客户端发送数据
    void on_autoCB_clicked(bool checked);//选择自动发送还是手动发送
    void on_sendMsgBt_clicked();//发送信息

    //服务器
    void accept_connect();//与newconnection信号关联
    void recv_data();   //接收数据
    void auto_time_send();//定时器定时发送数据
    void client_disconnect();//关联掉线信号
    void connect_suc();//监测客户端连接成功信号
    void on_btn_open_clicked();//打开按钮事件

    void on_clearRcvBt_clicked();
    void on_clearSendBt_clicked();
    void updateClientProgress(qint64 numBytes);

private:
    Ui::TcpApp *ui;
    QTimer *mTimer;//定时发送数据
    QTcpServer *mServer;
    QTcpSocket *mSocket;
    QVector<QTcpSocket*> clients;//存储所有在线客户端（容器）

    bool isServer;//标志位，true为服务器，false为客户端

    //保存接收和发送数据的字节数
    quint64 recvSize;
    quint64 sendSize;

    QString fileName;
    QFile *localFile;
    qint64 totalBytes;
    qint64 bytesWritten;
    qint64 payloadSize;
    qint64 bytesToWrite;
    qint64 fileNameSize;
    qint64 bytesReceived;
    QByteArray outBlock;
    QByteArray inBlock;

    qint16 onNum;
    bool isCheckServer;//判断是否选择了服务器
    bool isCheckClient;//判断是否选择了客户端
};

#endif // TCPAPP_H
