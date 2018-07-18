#include "ztcp2uartforwardthread.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <sys/socket.h>
#include <zgblpara.h>
#include <QtSerialPort/QSerialPort>
ZTcp2UartForwardThread::ZTcp2UartForwardThread()
{
    this->m_bExitFlag=false;
}
qint32 ZTcp2UartForwardThread::ZStartThread()
{
    this->m_bExitFlag=false;
    this->start();
    return 0;
}
qint32 ZTcp2UartForwardThread::ZStopThread()
{
    this->m_bExitFlag=true;
    return 0;
}
void ZTcp2UartForwardThread::run()
{
    QSerialPort *uart=new QSerialPort;
    uart->setPortName(gGblPara.m_uartName);
    uart->setBaudRate(QSerialPort::Baud115200);
    uart->setDataBits(QSerialPort::Data8);
    uart->setStopBits(QSerialPort::OneStop);
    uart->setParity(QSerialPort::NoParity);
    uart->setFlowControl(QSerialPort::NoFlowControl);
    if(!uart->open(QIODevice::ReadWrite))
    {
        qDebug()<<"<error>:Tcp2Uart cannot open uart device.";
        //此处设置全局请求退出标志,请求其他线程退出.
        gGblPara.m_bGblRst2Exit=true;
        delete uart;
        return;
    }

    qDebug()<<"<MainLoop>:Tcp2UartThread starts.";
    while(!gGblPara.m_bGblRst2Exit || !this->m_bExitFlag)
    {
        QTcpServer *tcpServer=new QTcpServer;
        int on=1;
        int sockFd=tcpServer->socketDescriptor();
        setsockopt(sockFd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
        if(!tcpServer->listen(QHostAddress::Any,TCP_PORT_FORWARD))
        {
            qDebug()<<"<error>: Tcp2Uart server listen error on port:"<<TCP_PORT_FORWARD;
            this->sleep(3);
            delete tcpServer;
            continue;
        }
        qDebug()<<"<Tcp2Uart>: listen on tcp "<<TCP_PORT_FORWARD;
        //wait until get a new connection.
        while(!gGblPara.m_bGblRst2Exit || !this->m_bExitFlag)
        {
            //qDebug()<<"wait for tcp connection";
            if(tcpServer->waitForNewConnection(1000*10))
            {
                break;
            }
        }

        QTcpSocket *tcpSocket=tcpServer->nextPendingConnection();
        if(tcpSocket)
        {
            //客户端连接上后，就判断服务监听端，这样只允许一个tcp连接.
            tcpServer->close();
            //设置连接标志.
            gGblPara.m_bTcp2UartConnected=true;
            qDebug()<<"Tcp2Uart connected.";

            //向客户端发送音频数据包.
            while(!gGblPara.m_bGblRst2Exit || !this->m_bExitFlag)
            {
                //read data from tcp and write it to uart.
                if(tcpSocket->waitForReadyRead(100))//100ms.
                {
                    QByteArray baFromTcp=tcpSocket->readAll();
                    if(uart->write(baFromTcp)<0)
                    {
                        qDebug()<<"<error>:Tcp2Uart failed to forward tcp to uart:"<<uart->errorString();
                        break;
                    }
                    uart->waitForBytesWritten(1000);
                    gGblPara.m_nTcp2UartBytes+=baFromTcp.size();
                }
                //read data from uart and write data to tcp.
                if(uart->waitForReadyRead(100))//100ms.
                {
                    QByteArray baFromUart=uart->readAll();
                    if(tcpSocket->write(baFromUart)<0)
                    {
                        qDebug()<<"<error>:Tcp2Uart failed to forward uart to tcp:"<<tcpSocket->errorString();
                        break;
                    }
                    tcpSocket->waitForBytesWritten(1000);
                    gGblPara.m_nUart2TcpBytes+=baFromUart.size();
                }

                //check tcp socket state.
                if(tcpSocket->state()!=QAbstractSocket::ConnectedState)
                {
                    qDebug()<<"<warning>:Tcp2Uart socket broken.";
                    break;
                }
                this->usleep(1000);//1000us.
            }

            //设置连接标志.
            gGblPara.m_bTcp2UartConnected=false;
            qDebug()<<"Tcp2Uart disconnected.";
        }
        delete tcpServer;
        tcpServer=NULL;
    }
    //退出主循环,做清理工作.
    uart->close();
    delete uart;
    qDebug()<<"<MainLoop>:Tcp2UartThread ends.";
    //此处设置本线程退出标志.
    //同时设置全局请求退出标志，请求其他线程退出.
    gGblPara.m_bTcp2UartThreadExitFlag=true;
    gGblPara.m_bGblRst2Exit=true;
    emit this->ZSigThreadFinished();
    return;
}
