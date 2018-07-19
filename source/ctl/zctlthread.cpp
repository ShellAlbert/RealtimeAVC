#include "zctlthread.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <sys/socket.h>
#include <zgblpara.h>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
ZCtlThread::ZCtlThread()
{

}
qint32 ZCtlThread::ZStartThread()
{
    this->m_bExitFlag=false;
    this->start();
    return 0;
}
qint32 ZCtlThread::ZStopThread()
{
    this->m_bExitFlag=true;
    return 0;
}
void ZCtlThread::run()
{
    while(!gGblPara.m_bGblRst2Exit && !this->m_bExitFlag)
    {
        QTcpServer *tcpServer=new QTcpServer;
        int on=1;
        int sockFd=tcpServer->socketDescriptor();
        setsockopt(sockFd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
        if(!tcpServer->listen(QHostAddress::Any,TCP_PORT_CTL))
        {
            qDebug()<<"<error>: Ctl server listen error on port:"<<TCP_PORT_CTL;
            this->sleep(3);
            delete tcpServer;
            continue;
        }
        qDebug()<<"<Ctl>: listen on tcp "<<TCP_PORT_CTL;
        //wait until get a new connection.
        while(!gGblPara.m_bGblRst2Exit && !this->m_bExitFlag)
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
            gGblPara.m_bCtlClientConnected=true;
            qDebug()<<"Ctl connected.";

            qint32 nJsonLen=0;
            //向客户端发送音频数据包.
            while(!gGblPara.m_bGblRst2Exit && !this->m_bExitFlag)
            {
                //read data from tcp and write it to uart.
                if(tcpSocket->waitForReadyRead(1000))//1000ms.
                {
                    //先读4字节的数据长度
                    if(0==nJsonLen && tcpSocket->bytesAvailable()>4)
                    {
                        QByteArray baJsonLen=tcpSocket->read(4);
                        if(baJsonLen.isEmpty())
                        {
                            qDebug()<<"<error>:error when read Ctl socket.";
                            break;
                        }
                        nJsonLen=QByteArrayToqint32(baJsonLen);
                    }
                    //如果数据长度为0,则等待下一次再读取
                    if(nJsonLen<=0)
                    {
                        continue;
                    }

                    if(tcpSocket->bytesAvailable()>=nJsonLen)
                    {
                        QByteArray baJsonData=tcpSocket->read(nJsonLen);
                        if(baJsonData.isEmpty())
                        {
                            qDebug()<<"<error>:error when read Ctl socket.";
                            break;
                        }
                        QJsonParseError jsonErr;
                        QJsonDocument jsonDoc=QJsonDocument::fromJson(baJsonData,&jsonErr);
                        if(jsonErr.error==QJsonParseError::NoError)
                        {
                            this->ZParseJson(jsonDoc);
                        }
                        nJsonLen=0;//reset it.
                    }
                }
                if(tcpSocket->state()!=QAbstractSocket::ConnectedState)
                {
                    break;
                }
            }
            tcpSocket->close();

            //设置连接标志.
            gGblPara.m_bCtlClientConnected=false;
            qDebug()<<"Ctl disconnected.";
        }
        delete tcpServer;
        tcpServer=NULL;
    }
}
void ZCtlThread::ZParseJson(const QJsonDocument &jsonDoc)
{
    if(!jsonDoc.isObject())
    {
        qDebug()<<"<error>:failed to parse json.";
        return;
    }
    QJsonObject jsonObj=jsonDoc.object();
    if(jsonObj.contains("ImgPro"))
    {
        QJsonValue val=jsonObj.take("ImgPro");
        if(val.isString())
        {
            QString method=val.toVariant().toString();
            if(method=="on")
            {
                //设置图像比对开启标志位.
                //这将引起采集线程向Process队列扔图像数据.
                //从而ImgProcess线程解除等待开始处理图像.
                gGblPara.m_bJsonImgPro=true;
            }else if(method=="off")
            {
                //设置图像比对暂停标志位.
                //这将引起采集线程不再向Process队列扔图像数据.
                //使得ImgProcess线程等待信号量从而暂停.
                gGblPara.m_bJsonImgPro=false;
            }else if(method=="query")
            {
                //仅用于查询当前状态.
            }
        }
    }
    if(jsonObj.contains("RTC"))
    {
        QJsonValue val=jsonObj.take("RTC");
        if(val.isString())
        {
            QString rtcStr=val.toVariant().toString();
            qDebug()<<"RTC:"<<rtcStr;
        }
    }
    if(jsonObj.contains("DeNoise"))
    {
        QJsonValue val=jsonObj.take("DeNoise");
        if(val.isString())
        {
            QString deNoise=val.toVariant().toString();
            qDebug()<<"deNoise:"<<deNoise;
            if(deNoise=="off")
            {

            }else if(deNoise=="RNNoise")
            {

            }else if(deNoise=="WebRTC")
            {

            }else if(deNoise=="Bevis")
            {

            }
        }
    }
    if(jsonObj.contains("BevisGrade"))
    {
        QJsonValue val=jsonObj.take("BevisGrade");
        if(val.isString())
        {
            QString bevisGrade=val.toVariant().toString();
            qDebug()<<"bevisGrade:"<<bevisGrade;
            if(bevisGrade=="1")
            {

            }else if(bevisGrade=="2")
            {

            }else if(bevisGrade=="3")
            {

            }else if(bevisGrade=="4")
            {

            }
        }
    }
    if(jsonObj.contains("DGain"))
    {
        QJsonValue val=jsonObj.take("DGain");
        if(val.isString())
        {
            QString dGain=val.toVariant().toString();
            qDebug()<<"bevisGrade:"<<dGain;
            qDebug()<<dGain.toInt();
        }
    }
    if(jsonObj.contains("FlushUI"))
    {
        QJsonValue val=jsonObj.take("FlushUI");
        if(val.isString())
        {
            QString flushUI=val.toVariant().toString();
            if(flushUI=="on")
            {
                gGblPara.m_bJsonFlushUI=true;
            }else if(flushUI=="off")
            {
                gGblPara.m_bJsonFlushUI=false;
            }else if(flushUI=="query")
            {
                //仅用于查询当前状态.
            }
        }
    }
}
