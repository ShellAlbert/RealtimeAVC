#include "zctlthread.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <sys/socket.h>
#include <zgblpara.h>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDateTime>
#include <QProcess>
#if 1
ZJsonThread::ZJsonThread(qintptr handle,QObject *parent):QThread(parent)
{
    this->m_handle=handle;
    this->m_tcpSocket=NULL;
}
void ZJsonThread::run()
{
    qDebug()<<"hello test";
    this->m_tcpSocket=new QTcpSocket;
    if(!this->m_tcpSocket->setSocketDescriptor(this->m_handle))
    {
        qDebug()<<"<Error>:"<<this->m_tcpSocket->errorString();
        return;
    }
    //设置连接标志.
    gGblPara.m_bCtlClientConnected=true;
    this->m_nJsonLen=0;

    QObject::connect(this->m_tcpSocket,SIGNAL(readyRead()),this,SLOT(ZSlotReadSocketData()),Qt::DirectConnection);
    QObject::connect(this->m_tcpSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(ZSlotSocketErr(QAbstractSocket::SocketError)),Qt::DirectConnection);

    //enter event-loop until exit() is called.
    this->exec();

    QObject::disconnect(this->m_tcpSocket,SIGNAL(readyRead()),this,SLOT(ZSlotReadSocketData()));
    QObject::disconnect(this->m_tcpSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(ZSlotSocketErr(QAbstractSocket::SocketError)));
    this->m_tcpSocket->close();
    delete this->m_tcpSocket;
    this->m_tcpSocket=NULL;

    //设置断开标志.
    gGblPara.m_bCtlClientConnected=false;
    return;
}
void ZJsonThread::ZSlotReadSocketData()
{
    //先读4字节的数据长度
    if(0==this->m_nJsonLen && this->m_tcpSocket->bytesAvailable()>4)
    {
        QByteArray baJsonLen=this->m_tcpSocket->read(4);
        if(baJsonLen.isEmpty())
        {
            qDebug()<<"<Error>:error when read json len.";
            //exit the event-loop.
            this->exit(-1);
            return;
        }
        this->m_nJsonLen=QByteArrayToqint32(baJsonLen);
    }
    qDebug()<<this->m_nJsonLen;
    //如果数据长度为0,则等待下一次再读取
    if(this->m_nJsonLen<=0)
    {
        return;
    }

    if(this->m_tcpSocket->bytesAvailable()>=this->m_nJsonLen)
    {
        QByteArray baJsonData=this->m_tcpSocket->read(this->m_nJsonLen);
        if(baJsonData.isEmpty())
        {
            qDebug()<<"<error>:error when read json data.";
            //exit the event-loop.
            this->exit(-1);
            return;
        }
        qDebug()<<baJsonData;
        QJsonParseError jsonErr;
        QJsonDocument jsonDoc=QJsonDocument::fromJson(baJsonData,&jsonErr);
        if(jsonErr.error==QJsonParseError::NoError)
        {
            QByteArray baFeedBack=this->ZParseJson(jsonDoc);
            if(baFeedBack.size()>0)
            {
                this->m_tcpSocket->write(baFeedBack);
                this->m_tcpSocket->waitForBytesWritten(100);
            }
        }

        //处理完一帧完整的数据后，重置长度.
        this->m_nJsonLen=0;
    }
}
void ZJsonThread::ZSlotSocketErr(QAbstractSocket::SocketError err)
{
    qDebug()<<err;
    //call exit() to exit event-loop.
    this->exit(-1);
}
QByteArray ZJsonThread::ZParseJson(const QJsonDocument &jsonDoc)
{
    bool bWrCfgFileFlag=false;
    QJsonObject jsonObjFeedBack;
    if(jsonDoc.isObject())
    {
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
                jsonObjFeedBack.insert("ImgPro",gGblPara.m_bJsonImgPro?"on":"false");
            }
        }
        if(jsonObj.contains("RTC"))
        {
            QJsonValue val=jsonObj.take("RTC");
            if(val.isString())
            {
                QString rtcStr=val.toVariant().toString();
                qDebug()<<"RTC:"<<rtcStr;
                QString cmdSetRtc=QString("date -s %1").arg(rtcStr);
                qDebug()<<"cmdSetRtc:"<<cmdSetRtc;
                QProcess::startDetached(cmdSetRtc);
                QProcess::startDetached("hwclock -w");
                QProcess::startDetached("sync");
                jsonObjFeedBack.insert("RTC",QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss"));
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
                    gGblPara.m_audio.m_nDeNoiseMethod=0;
                }else if(deNoise=="RNNoise")
                {
                    gGblPara.m_audio.m_nDeNoiseMethod=1;
                }else if(deNoise=="WebRTC")
                {
                    gGblPara.m_audio.m_nDeNoiseMethod=2;
                }else if(deNoise=="Bevis")
                {
                    gGblPara.m_audio.m_nDeNoiseMethod=3;
                }else if(deNoise=="query")
                {
                    //仅用于查询当前状态.
                }
                switch(gGblPara.m_audio.m_nDeNoiseMethod)
                {
                case 0:
                    jsonObjFeedBack.insert("DeNoise","off");
                    break;
                case 1:
                    jsonObjFeedBack.insert("DeNoise","RNNoise");
                    break;
                case 2:
                    jsonObjFeedBack.insert("DeNoise","WebRTC");
                    break;
                case 3:
                    jsonObjFeedBack.insert("DeNoise","Bevis");
                    break;
                default:
                    break;
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
                    gGblPara.m_audio.m_nBevisGrade=1;
                }else if(bevisGrade=="2")
                {
                    gGblPara.m_audio.m_nBevisGrade=2;
                }else if(bevisGrade=="3")
                {
                    gGblPara.m_audio.m_nBevisGrade=3;
                }else if(bevisGrade=="4")
                {
                    gGblPara.m_audio.m_nBevisGrade=4;
                }else if(bevisGrade=="query")
                {
                    //仅用于查询当前状态.
                }
                switch(gGblPara.m_audio.m_nBevisGrade)
                {
                case 1:
                    jsonObjFeedBack.insert("BevisGrade","1");
                    break;
                case 2:
                    jsonObjFeedBack.insert("BevisGrade","2");
                    break;
                case 3:
                    jsonObjFeedBack.insert("BevisGrade","3");
                    break;
                case 4:
                    jsonObjFeedBack.insert("BevisGrade","4");
                    break;
                default:
                    break;
                }
            }
        }
        if(jsonObj.contains("DGain"))
        {
            QJsonValue val=jsonObj.take("DGain");
            if(val.isString())
            {
                QString dGain=val.toVariant().toString();
                if(dGain=="query")
                {
                    jsonObjFeedBack.insert("DGain",QString::number(gGblPara.m_audio.m_nGaindB));
                }else{
                    qint32 dGain=val.toVariant().toInt();
                    qDebug()<<"bevisGrade:"<<dGain;
                    gGblPara.m_audio.m_nGaindB=dGain;
                    jsonObjFeedBack.insert("DGain",QString::number(gGblPara.m_audio.m_nGaindB));
                }
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
                    gGblPara.m_bJsonFlushUIWav=true;
                    gGblPara.m_bJsonFlushUIImg=true;

                }else if(flushUI=="off")
                {
                    gGblPara.m_bJsonFlushUIWav=false;
                    gGblPara.m_bJsonFlushUIImg=false;
                }else if(flushUI=="query")
                {
                    //仅用于查询当前状态.
                }
                if(gGblPara.m_bJsonFlushUIWav && gGblPara.m_bJsonFlushUIImg)
                {
                    jsonObjFeedBack.insert("FlushUI","on");
                }else{
                    jsonObjFeedBack.insert("FlushUI","off");
                }
            }
        }
        if(jsonObj.contains("Cam1CenterXY"))
        {
            QJsonValue val=jsonObj.take("Cam1CenterXY");
            if(val.isString())
            {
                QString cam1CenterXY=val.toVariant().toString();
                if(cam1CenterXY=="query")
                {
                    jsonObjFeedBack.insert("Cam1CenterXY",QString("%1,%2").arg(gGblPara.m_calibrateX1).arg(gGblPara.m_calibrateY1));
                }else{
                    QStringList xyList=cam1CenterXY.split(",");
                    if(xyList.size()!=2)
                    {
                        qDebug()<<"<Error>:failed to parse out (x,y) in Cam1CenterXY json.";
                    }else{
                        qint32 x=xyList.at(0).toInt();
                        qint32 y=xyList.at(1).toInt();
                        gGblPara.m_calibrateX1=x;
                        gGblPara.m_calibrateY1=y;
                        bWrCfgFileFlag=true;
                        jsonObjFeedBack.insert("Cam1CenterXY",QString("%1,%2").arg(gGblPara.m_calibrateX1).arg(gGblPara.m_calibrateY1));
                    }
                }
            }
        }
        if(jsonObj.contains("Cam2CenterXY"))
        {
            QJsonValue val=jsonObj.take("Cam2CenterXY");
            if(val.isString())
            {
                QString cam2CenterXY=val.toVariant().toString();
                if(cam2CenterXY=="query")
                {
                    jsonObjFeedBack.insert("Cam2CenterXY",QString("%1,%2").arg(gGblPara.m_calibrateX2).arg(gGblPara.m_calibrateY2));
                }else{
                    QStringList xyList=cam2CenterXY.split(",");
                    if(xyList.size()!=2)
                    {
                        qDebug()<<"<Error>:failed to parse out (x,y) in Cam1CenterXY json.";
                    }else{
                        qint32 x=xyList.at(0).toInt();
                        qint32 y=xyList.at(1).toInt();
                        gGblPara.m_calibrateX2=x;
                        gGblPara.m_calibrateY2=y;
                        bWrCfgFileFlag=true;
                        jsonObjFeedBack.insert("Cam2CenterXY",QString("%1,%2").arg(gGblPara.m_calibrateX2).arg(gGblPara.m_calibrateY2));
                    }
                }
            }
        }
        if(jsonObj.contains("Accumulated"))
        {
            QJsonValue val=jsonObj.take("Accumulated");
            if(val.isString())
            {
                QString str=val.toVariant().toString();
                if(str=="query")
                {
                    jsonObjFeedBack.insert("Accumulated",QString::number(gGblPara.m_nAccumulatedSec));
                }else{
                    qDebug()<<"<Error>:unknow cmd in Accumulated json.";
                }
            }
        }
        if(bWrCfgFileFlag)
        {
            gGblPara.writeCfgFile();
        }
    }else{
        qDebug()<<"<Error>:CtlThread,failed to parse json.";
    }
    QJsonDocument jsonDocFeedBack;
    jsonDocFeedBack.setObject(jsonObjFeedBack);
    QByteArray baFeedBack=jsonDocFeedBack.toJson();
    return baFeedBack;
}
ZCtlServer::ZCtlServer()
{
    this->m_jsonThread=NULL;
    int on=1;
    int sockFd=this->socketDescriptor();
    setsockopt(sockFd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
}
qint32 ZCtlServer::ZStartServer()
{
    if(!this->listen(QHostAddress::Any,TCP_PORT_CTL))
    {
        qDebug()<<"<Error>: Ctl server listen error on port:"<<TCP_PORT_CTL;
        return -1;
    }
    return 0;
}
void ZCtlServer::incomingConnection(qintptr socketDescriptor)
{
    //同一时刻只允许一个客户端连接，当连接上时，则关闭服务端
    this->close();
    qDebug()<<"close server.";
    //开启处理线程.
    this->m_jsonThread=new ZJsonThread(socketDescriptor);
    QObject::connect(this->m_jsonThread,SIGNAL(finished()),this,SLOT(ZSlotJsonThreadFinished()),Qt::AutoConnection);
    this->m_jsonThread->start();
}
void ZCtlServer::ZSlotJsonThreadFinished()
{
    delete this->m_jsonThread;
    this->m_jsonThread=NULL;
    //重新开启监听.
    this->ZStartServer();
    qDebug()<<"reopen server";
}
#endif

///////////////////////////////////////////////////////
/// \brief ZCtlThread::ZCtlThread
#if 0
ZCtlThread::ZCtlThread()
{
    this->m_bExitFlag=false;
    this->m_bCleanup=false;
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
    qDebug()<<"<MainLoop>:"<<_CURRENT_DATETIME_<<"CtlThread starts"<<TCP_PORT_CTL<<".";
    this->m_bCleanup=false;
    while(!gGblPara.m_bGblRst2Exit && !this->m_bExitFlag)
    {
        this->m_tcpServer=new QTcpServer(this);
        int on=1;
        int sockFd=this->m_tcpServer->socketDescriptor();
        setsockopt(sockFd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
        if(!this->m_tcpServer->listen(QHostAddress::Any,TCP_PORT_CTL))
        {
            qDebug()<<"<Error>: Ctl server listen error on port:"<<TCP_PORT_CTL;
            delete this->m_tcpServer;
            break;
        }
        QObject::connect(this->m_tcpServer,SIGNAL(newConnection()),this,SLOT(ZSlotNewConnection()));
        QObject::connect(this->m_tcpServer,SIGNAL(acceptError(QAbstractSocket::SocketError)),this,SLOT(ZSlotAcceptErr(QAbstractSocket::SocketError)));
        //enter event-loop until exit() is called.
        this->exec();
        QObject::disconnect(this->m_tcpServer,SIGNAL(newConnection()),this,SLOT(ZSlotNewConnection()));
        QObject::disconnect(this->m_tcpServer,SIGNAL(acceptError(QAbstractSocket::SocketError)),this,SLOT(ZSlotAcceptErr(QAbstractSocket::SocketError)));
        delete this->m_tcpServer;
        this->m_tcpServer=NULL;
    }
    qDebug()<<"<MainLoop>:"<<_CURRENT_DATETIME_<<"CtlThread ends.";
    emit this->ZSigThreadExited();
    this->m_bCleanup=true;
    return;
    /////////////////////////////////////////////////////
#if 0
    qDebug()<<"<MainLoop>:"<<_CURRENT_DATETIME_<<"CtlThread starts"<<TCP_PORT_CTL<<".";
    this->m_bCleanup=false;
    while(!gGblPara.m_bGblRst2Exit && !this->m_bExitFlag)
    {
        QTcpServer *tcpServer=new QTcpServer;
        int on=1;
        int sockFd=tcpServer->socketDescriptor();
        setsockopt(sockFd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
        if(!tcpServer->listen(QHostAddress::Any,TCP_PORT_CTL))
        {
            qDebug()<<"<Error>: Ctl server listen error on port:"<<TCP_PORT_CTL;
            delete tcpServer;
            break;
        }

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
                    qDebug()<<nJsonLen;
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
                        qDebug()<<baJsonData;
                        QJsonParseError jsonErr;
                        QJsonDocument jsonDoc=QJsonDocument::fromJson(baJsonData,&jsonErr);
                        if(jsonErr.error==QJsonParseError::NoError)
                        {
                            QByteArray baFeedBack=this->ZParseJson(jsonDoc);
                            if(baFeedBack.size()>0)
                            {
                                tcpSocket->write(baFeedBack);
                                tcpSocket->waitForBytesWritten(1000);
                            }
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
        }
        delete tcpServer;
        tcpServer=NULL;
    }
    qDebug()<<"<MainLoop>:"<<_CURRENT_DATETIME_<<"CtlThread ends.";
    emit this->ZSigThreadExited();
    this->m_bCleanup=true;
    return;
#endif
}
void ZCtlThread::ZSlotAcceptErr(QAbstractSocket::SocketError err)
{
    qDebug()<<err;
    //exit the event loop.
    this->exit(-1);
}
void ZCtlThread::ZSlotNewConnection()
{
    this->m_tcpSocket=this->m_tcpServer->nextPendingConnection();
    if(this->m_tcpSocket)
    {
        //客户端连接上后，就判断服务监听端，这样只允许一个tcp连接.
        this->m_tcpServer->close();
        //设置连接标志.
        gGblPara.m_bCtlClientConnected=true;

        //initial json length.
        this->m_nJsonLen=0;
        //make connection.
        QObject::connect(this->m_tcpSocket,SIGNAL(readyRead()),this,SLOT(ZSlotReadSocketData()));
        QObject::connect(this->m_tcpSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(ZSlotSocketErr(QAbstractSocket::SocketError)));
    }
}
void ZCtlThread::ZSlotReadSocketData()
{

}
void ZCtlThread::ZSlotSocketErr(QAbstractSocket::SocketError err)
{
    qDebug()<<err;
    //close socket.
    QObject::disconnect(this->m_tcpSocket,SIGNAL(readyRead()),this,SLOT(ZSlotReadSocketData()));
    QObject::disconnect(this->m_tcpSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(ZSlotSocketErr(QAbstractSocket::SocketError)));

    this->m_tcpSocket->close();
    delete this->m_tcpSocket;
    this->m_tcpSocket=NULL;
    //设置断开标志.
    gGblPara.m_bCtlClientConnected=false;
    //exit the event-loop.
    this->exit(-1);
}

bool ZCtlThread::ZIsExitCleanup()
{
    return this->m_bCleanup;
}
#endif
