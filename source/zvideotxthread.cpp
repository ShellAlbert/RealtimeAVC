#include "zvideotxthread.h"
#include <zgblpara.h>
#include <sys/socket.h>
#include <iostream>
#include "rtpsession.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
#include "rtperrors.h"
#include "rtplibraryversion.h"
using namespace jrtplib;
//  RTP:
//  -----------------------------------------------------------------
//  0               8              16              24              32
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  | V |P|X|  CC   |M|     PT      |               SN              |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                           Timestamp                           |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                             SSRC                              |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                             CSRC                              |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                             ...                               |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

//  NALU:
//  -----------------
//  0               8
//  +-+-+-+-+-+-+-+-+
//  |F|NRI|   Type  |
//  +-+-+-+-+-+-+-+-+

#define MAXLEN  (RTP_DEFAULTPACKETSIZE - 100)

void checkerror(int rtperr)
{
    if (rtperr < 0)
    {
        std::cout << "ERROR: " << RTPGetErrorString(rtperr) << std::endl;
        exit(-1);
    }
}

static int findStartCode1(unsigned char *buf)
{
    if(buf[0]!=0 || buf[1]!=0 || buf[2]!=1)                // 判断是否为0x000001,如果是返回1
        return 0;
    else
        return 1;
}
static int findStartCode2(unsigned char *buf)
{
    if(buf[0]!=0 || buf[1]!=0 || buf[2]!=0 || buf[3]!=1)   // 判断是否为0x00000001,如果是返回1
        return 0;
    else
        return 1;
}
void naluPrintf(unsigned char *buf, unsigned int len, unsigned char type, unsigned int count)
{
    unsigned int i=0;

    printf("NALU type=%d num=%d len=%d : \n", type, count, len);

#if 0
    for(i=0; i<len; i++)
    {
        printf(" %02X", buf[i]);
        if(i%32 == 31)
            printf("\n");
    }
    printf("\n");
#endif
}
void rtpPrintf(unsigned char *buf, unsigned int len)
{
    unsigned int i=0;

    printf("RTP len=%d : \n", len);

    for(i=0; i<len; i++)
    {
        printf(" %02X", buf[i]);
        if(i%32 == 31)
            printf("\n");
    }

    printf("\n");
}

ZVideoTxThread::ZVideoTxThread()
{
    this->m_queue=NULL;
    this->m_semaUsed=NULL;
    this->m_semaFree=NULL;
    this->m_bExitFlag=false;
}
qint32 ZVideoTxThread::ZBindQueue(QQueue<QByteArray> *queue,QSemaphore *semaUsed,QSemaphore *semaFree)
{
    if(NULL==queue||NULL==semaUsed||NULL==semaFree)
    {
        qDebug()<<"<error>:invalid parameters,bind queue failed.";
        return -1;
    }
    this->m_queue=queue;
    this->m_semaUsed=semaUsed;
    this->m_semaFree=semaFree;
    return 0;
}
qint32 ZVideoTxThread::ZStartThread()
{
    this->m_bExitFlag=false;
    this->start();
    return 0;
}
qint32 ZVideoTxThread::ZStopThread()
{
    this->m_bExitFlag=true;
    return 0;
}
void ZVideoTxThread::run()
{
#if 0
    RTPSession session;
    qDebug()<<"JRTP version:"<<RTPLibraryVersion::GetVersion().GetVersionString().c_str();
    RTPUDPv4TransmissionParams transParam;
    transParam.SetPortbase(1234);


    RTPSessionParams sessionPara;
    // IMPORTANT: The local timestamp unit MUST be set, otherwise
    //            RTCP Sender Report info will be calculated wrong
    // In this case, we'll be sending 10 samples each second, so we'll
    // put the timestamp unit to (1.0/10.0)
    sessionPara.SetOwnTimestampUnit(1.0/10.0);
    sessionPara.SetAcceptOwnPackets(true);
    int status=session.Create(sessionPara,&transParam);
    checkerror(status);

    //set the destination ip & port.
    RTPIPv4Address addr(inet_addr("127.0.0.1"),ntohl(1200));
    status=session.AddDestination(addr);
    checkerror(status);

    session.SetDefaultPayloadType(96);//96 for h264 type.
    session.SetDefaultMark(false);
    session.SetDefaultTimestampIncrement(90000.0/10.0);
    RTPTime delay(0.040);
    RTPTime startTime=RTPTime::CurrentTime();
#endif
    qDebug()<<"<MainLoop>:VideoTxThread starts.";
    while(!gGblPara.m_bGblRst2Exit)
    {
        QTcpServer *tcpServer=new QTcpServer;
        int on=1;
        int sockFd=tcpServer->socketDescriptor();
        setsockopt(sockFd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
        if(!tcpServer->listen(QHostAddress::Any,TCP_PORT_VIDEO))
        {
            qDebug()<<"<error>: tcp server error listen on port"<<TCP_PORT_VIDEO;
            this->sleep(3);
            delete tcpServer;
            continue;
        }
        //wait until get a new connection.
        while(!gGblPara.m_bGblRst2Exit)
        {
            //qDebug()<<"wait for tcp connection";
            if(tcpServer->waitForNewConnection(1000*10))
            {
                break;
            }
        }
        if(!gGblPara.m_bGblRst2Exit)
        {
            QTcpSocket *tcpSocket=tcpServer->nextPendingConnection();
            if(NULL==tcpSocket)
            {
                qDebug()<<"<error>: failed to get next pending connection.";
            }else{
                //qDebug()<<"new connection,close tcp server.";
                //客户端连接上后，就判断服务监听端，这样只允许一个tcp连接.
                tcpServer->close();
                //设置连接标志，这样编码器线程就会开始工作.
                gGblPara.m_bTcpClientConnected=true;
                qDebug()<<"connected.";

                //向客户端发送音频数据包.
                while(!gGblPara.m_bGblRst2Exit)
                {
                    //fetch data from tcp queue.
                    QByteArray baH264Data;
                    this->m_semaUsed->acquire();//已用信号量减1.
                    baH264Data=this->m_queue->dequeue();
                    this->m_semaFree->release();//空闲信号量加1.

                    //Audio Packet format: pkt len + pkt data.
                    QByteArray baH264PktLen=qint32ToQByteArray(baH264Data.size());
//                    if(tcpSocket->write(baH264PktLen)<0)
//                    {
//                        qDebug()<<"<error>:socket write error,break it.";
//                        break;
//                    }
                    baH264Data.prepend(baH264PktLen);
                    if(tcpSocket->write(baH264Data)<0)
                    {
                        qDebug()<<"<error>:socket write error,break it.";
                        break;
                    }
                    tcpSocket->waitForBytesWritten(1000);
                    //qDebug()<<"tx h264:"<<baH264Data.size();
                    this->usleep(1000);//1000us.
                }
                //设置连接标志，这样编码器线程就会停止工作.
                gGblPara.m_bTcpClientConnected=false;
                qDebug()<<"disconnected.";
            }
        }
        delete tcpServer;
    }
    qDebug()<<"<MainLoop>:VideoTxThread ends.";
    //此处设置本线程退出标志.
    //同时设置全局请求退出标志，请求其他线程退出.
    gGblPara.m_bVideoTxThreadExitFlag=true;
    gGblPara.m_bGblRst2Exit=true;
    emit this->ZSigThreadFinished();
}
