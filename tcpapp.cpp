#include "tcpapp.h"
#include "ui_tcpapp.h"

TcpApp::TcpApp(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TcpApp), onNum(0)
{
    ui->setupUi(this);
    this->setWindowTitle(tr("Tcp套接字"));

    recvSize = 0;
    sendSize = 0;
    //初始化定时器
    mTimer = new QTimer();
    connect(mTimer, SIGNAL(timeout()), this, SLOT(auto_time_send()));
}

TcpApp::~TcpApp()
{
    delete ui;
}

void TcpApp::accept_connect()//接受连接
{
    mSocket = mServer->nextPendingConnection();//返回客户端连接通信的套接字

    //关联接收数据信号
    connect(mSocket, SIGNAL(readyRead()), this, SLOT(recv_data()));
    //关联掉线信号
    connect(mSocket, SIGNAL(disconnected()), this, SLOT(client_disconnect()));

    //上线用户添加到客户列表容器
    clients.append(mSocket);
    //把用户添加到界面列表中
    QString ip = mSocket->peerAddress().toString().remove("::ffff:");//去除客户端中多余的字符
    ui->onlineUserList->addItem(ip);

    //在线数量添加
    onNum++;
    ui->onlineUserCout->setText(QString::number(onNum));//显示在线数
}

void TcpApp::recv_data()//接收数据
{
    QDataStream in(mSocket);
    in.setVersion(QDataStream::Qt_4_0);

    if (bytesReceived <= sizeof(qint64)*2)
    {
        if((mSocket->bytesAvailable() >= sizeof(qint64)*2) && (fileNameSize == 0))
        {

            in >> totalBytes >> fileNameSize;
            bytesReceived += sizeof(qint64) * 2;
        }
        if (fileNameSize == 0)
            goto text;
        if((mSocket->bytesAvailable() >= fileNameSize) && (fileNameSize != 0))
        {
            in >> fileName;
            bytesReceived += fileNameSize;
            localFile = new QFile(fileName);
            if (!localFile->open(QFile::WriteOnly))
            {
                qDebug() << "server: open file error!";
                return;
            }
        }
        else
            return;
    }
    if (bytesReceived < totalBytes)
    {
        bytesReceived += mSocket->bytesAvailable();
        inBlock = mSocket->readAll();
        localFile->write(inBlock);
        inBlock.resize(0);
    }

    if (bytesReceived == totalBytes)
    {
        mSocket->close();
        localFile->close();
        QString text = QString("接收文件 %1 成功").arg(fileName);
        QMessageBox::about(this, "提示", text);
        totalBytes = 0;
        bytesReceived = 0;
        fileNameSize = 0;
        fileName.clear();
        return;
    }
    else
        goto file;
text:{
        QTcpSocket *obj = (QTcpSocket*)sender();
        //获取发送数据段的IP
        QString ip = obj->peerAddress().toString();
        ip.remove("::::ffff:");
        QString msg = obj->readAll();
        ui->recieveList->addItem(ip+":"+msg);//显示接收到的数据
        recvSize += msg.size();//统计接收到的数据的字节数
        ui->receiveNumLabel->setText(QString::number(recvSize));
    }
    file:;
}

void TcpApp::client_disconnect()//断开连接
{
    QTcpSocket *obj = (QTcpSocket*)sender();//获取掉线对象
    if (isServer)
    {
        int row = clients.indexOf(obj);
        QListWidgetItem *item = ui->onlineUserList->takeItem(row);//从界面列表中去除找到的一行内容
        delete item;
        clients.remove(row);//从容器中删除对象

        //掉线时删除在线数量
        onNum--;
        ui->onlineUserCout->setText(QString::number(onNum));
    }
    else
        ui->StartBt->setEnabled(true);//断开连接的时候重新启用开始按钮
}

void TcpApp::connect_suc()//客户端连接成功
{
    ui->StartBt->setEnabled(false);//连接成功则禁用开始按钮
}

void TcpApp::auto_time_send()//定时器定时发送数据
{
    if (!fileName.isEmpty())
    {
        localFile = new QFile(fileName);
        if (!localFile->open(QFile::ReadOnly))
        {
            qDebug()<<"client: open file error!";
            return;
        }
        totalBytes = localFile->size();

        QDataStream sendOut(&outBlock, QIODevice::WriteOnly);
        sendOut.setVersion(QDataStream::Qt_4_0);
        QString currentFileName = fileName.right(fileName.size() - fileName.lastIndexOf('/') -1 );

        // 保留总大小信息空间、文件名大小信息空间，然后输入文件名
        sendOut << qint64(0) << qint64(0) << currentFileName;

        // 这里的总大小是总大小信息、文件名大小信息、文件名和实际文件大小的总和
        totalBytes += outBlock.size();
        sendOut.device()->seek(0);

        // 返回outBolock的开始，用实际的大小信息代替两个qint64(0)空间
        sendOut << totalBytes << qint64((outBlock.size() - sizeof(qint64)*2));

        // 发送完文件头结构后剩余数据的大小
        bytesToWrite = totalBytes - mSocket->write(outBlock);

        outBlock.resize(0);
        totalBytes = 0;
        bytesReceived = 0;
        fileNameSize = 0;
        fileName.clear();
    }
    else
    {
        quint64 len = mSocket->write(ui->sendMsgEdit->toPlainText().toUtf8());
        if (len > 0)
        {
            sendSize += len;//统计发送的字节数
            ui->sendNumLabel->setText(QString::number(sendSize));//把发送的字节数显示到sendNumLabel上

            outBlock.resize(0);
            totalBytes = 0;
            bytesReceived = 0;
            fileNameSize = 0;
            fileName.clear();
        }
    }
}

void TcpApp::updateClientProgress(qint64 numBytes)
{
    // 已经发送数据的大小
    bytesWritten += (int)numBytes;

    // 如果已经发送了数据
    if (bytesToWrite > 0)
    {
        // 每次发送payloadSize大小的数据，这里设置为64KB，如果剩余的数据不足64KB，
        // 就发送剩余数据的大小
        outBlock = localFile->read(qMin(bytesToWrite, payloadSize));

        // 发送完一次数据后还剩余数据的大小
        bytesToWrite -= (int)mSocket->write(outBlock);

        // 清空发送缓冲区
        outBlock.resize(0);
    }
    else
    { // 如果没有发送任何数据，则关闭文件
        localFile->close();
    }
    // 如果发送完毕
    if(bytesWritten == totalBytes)
    {
        localFile->close();
        mSocket->close();
        totalBytes = 0;
        bytesReceived = 0;
        fileNameSize = 0;
        fileName.clear();
    }
}

void TcpApp::on_serverRB_clicked()//选择作为服务器
{
    this->isCheckServer = true;
    this->isServer = true;
    //获取本地ip显示在IpEdit中
    ui->IpEdit->setText(QHostAddress(QHostAddress::LocalHost).toString());
    ui->IpEdit->setEnabled(false);//关闭ip输入编辑器
    this->isCheckClient = false;
    this->setWindowTitle(tr("Tcp服务器"));
}

void TcpApp::on_clientRB_clicked()//选择作为客户端
{
    this->isCheckClient = true;
    this->isServer = false;
    ui->IpEdit->setEnabled(true);
    this->isCheckServer = false;
    this->setWindowTitle(tr("Tcp客户端"));
}

void TcpApp::on_StartBt_clicked()//启动服务器或者连接服务器
{
    if (isServer)//服务器
    {
        totalBytes = 0;
        bytesReceived = 0;
        fileNameSize = 0;
        mServer = new QTcpServer();
        //关联新客户端连接信号
        connect(mServer, SIGNAL(newConnection()), this, SLOT(accept_connect()));
        mServer->listen(QHostAddress::Any, ui->PortEdit->text().toInt());
        ui->StartBt->setEnabled(false);//开始按钮禁用
        ui->btn_open->setEnabled(false);
    }
    if (isServer == false)
    {
        ui->btn_open->setEnabled(true);
        mSocket = new QTcpSocket();
        //监测连接成功信号
        connect(mSocket, SIGNAL(connected()), this, SLOT(connect_suc()));
        //监测发送文件内容
        connect(mSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(updateClientProgress(qint64)));
        //设置服务器的ip和端口号
        mSocket->connectToHost(ui->IpEdit->text(), ui->PortEdit->text().toInt());
        //关联接收数据信号
        connect(mSocket, SIGNAL(readyRead()), this, SLOT(recv_data()));
        //关联掉线信号
        connect(mSocket, SIGNAL(disconnected()), this, SLOT(client_disconnect()));
    }
    if (isCheckServer == false && isCheckClient == false)//如果两个都没选择
    {
        QMessageBox::warning(this, "提示", "请选择服务器或者客户端");
        ui->StartBt->setEnabled(true);
        ui->btn_open->setEnabled(true);
        return;
    }
    if (isCheckServer)//选择了服务器
    {
        if (ui->PortEdit->text().isEmpty() || ui->PortEdit->text() == "请输入端口号")
        {
            QMessageBox::warning(this, "提示", "请输入端口号");
            ui->StartBt->setEnabled(true);
            ui->btn_open->setEnabled(true);
            return;
        }
    }
    if (isCheckClient)//选择了客户端
    {
        if (ui->IpEdit->text().isEmpty() || ui->IpEdit->text() == "请输入ip" || ui->IpEdit->text() == "请输入端口号")
        {
            QMessageBox::warning(this, "提示", "请输入ip和端口号");
            ui->StartBt->setEnabled(true);
            ui->btn_open->setEnabled(true);
            return;
        }
    }
}

void TcpApp::on_closeBt_clicked()//关闭服务器或者断开
{
    if (isServer)//服务器
    {
        for (int i=0; i<clients.count(); i++)
        {
            clients.at(i)->close();//关闭所有客户端
        }
        //关闭所有服务器之后开始按钮才能启用
        mServer->close();
        ui->StartBt->setEnabled(true);
    }
    else
    {
        mSocket->close();//关闭客户端
        ui->StartBt->setEnabled(true);//启用开始按钮
    }
}

void TcpApp::on_onlineUserList_doubleClicked(const QModelIndex &index)//双击选择要发送的客户端
{
    mSocket = clients.at(index.row());
}

void TcpApp::on_autoCB_clicked(bool checked)//自动发送数据
{
    if (checked)
    {
        if (ui->autoTimeEidt->text().toInt() <= 0)
        {
            QMessageBox::warning(this, "提示", "请输入时间值ms");
            ui->autoCB->setChecked(false);//把按钮重新置于没选中的状态
            return;
        }
        mTimer->start(ui->autoTimeEidt->text().toInt());//启动定时器
    }
    else
    {
        mTimer->stop();//停止定时器
    }
}

void TcpApp::on_sendMsgBt_clicked()//自动按钮触发
{
        auto_time_send();
}

void TcpApp::on_clearRcvBt_clicked()//清空接收区
{
    ui->recieveList->clear();
    this->recvSize = 0;
    ui->receiveNumLabel->setText(QString::number(recvSize));
}

void TcpApp::on_clearSendBt_clicked()//清空发送区
{
    ui->sendMsgEdit->clear();
    this->sendSize = 0;
    ui->sendNumLabel->setText(QString::number(sendSize));
}

void TcpApp::on_btn_open_clicked()//打开文件
{
    fileName.clear();
    fileName = QFileDialog::getOpenFileName(this);
    if (fileName.isEmpty())
    {
        return;
    }
}
