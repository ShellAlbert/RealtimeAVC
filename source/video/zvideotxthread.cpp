#include "zvideotxthread.h"
#include <zgblpara.h>
#include <sys/socket.h>
#include <iostream>
#if 0
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
#endif
ZVideoTxThread::ZVideoTxThread(qint32 nTcpPort)
{
    this->m_rbH264=NULL;
    this->m_bExitFlag=false;
    this->m_nTcpPort=nTcpPort;
}
qint32 ZVideoTxThread::ZBindQueue(ZRingBuffer *rbH264)
{
    this->m_rbH264=rbH264;
    return 0;
}
qint32 ZVideoTxThread::ZStartThread()
{
    if(NULL==this->m_rbH264)
    {
        qDebug()<<"<Error>:VideoTxThread,no bind h264 queue,can not start.";
        return -1;
    }
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
    char *txBuffer=new char[BUFSIZE_1MB];
    qDebug()<<"<MainLoop>:VideoTxThread starts ["<<this->m_nTcpPort<<"].";
    while(!gGblPara.m_bGblRst2Exit && !this->m_bExitFlag)
    {
        QTcpServer *tcpServer=new QTcpServer;
        int on=1;
        int sockFd=tcpServer->socketDescriptor();
        setsockopt(sockFd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
        if(!tcpServer->listen(QHostAddress::Any,this->m_nTcpPort))
        {
            qDebug()<<"<Error>:VideoTx tcp server error listen on port"<<this->m_nTcpPort;
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
        if(!gGblPara.m_bGblRst2Exit && !this->m_bExitFlag)
        {
            QTcpSocket *tcpSocket=tcpServer->nextPendingConnection();
            if(NULL==tcpSocket)
            {
                qDebug()<<"<Error>: failed to get next pending connection.";
            }else{
                //qDebug()<<"new connection,close tcp server.";
                //客户端连接上后，就判断服务监听端，这样只允许一个tcp连接.
                tcpServer->close();
                //设置连接标志，这样编码器线程就会开始工作.
                switch(this->m_nTcpPort)
                {
                case TCP_PORT_VIDEO:
                    gGblPara.m_bVideoTcpConnected=true;
                    break;
                case TCP_PORT_VIDEO2:
                    gGblPara.m_bVideoTcpConnected2=true;
                    break;
                default:
                    break;
                }

                //向客户端发送音频数据包.
                while(!gGblPara.m_bGblRst2Exit && !this->m_bExitFlag)
                {
                    qint32 nH264Len=0;
                    //fetch data from h264 queue.
                    if(!this->m_rbH264->m_semaUsed->tryAcquire())//已用信号量减1.
                    {
                        this->usleep(VIDEO_THREAD_SCHEDULE_US);
                        continue;
                    }
//                    this->m_rbH264->m_semaUsed->acquire();
                    nH264Len=this->m_rbH264->ZGetElement((qint8*)txBuffer,BUFSIZE_1MB);
                    this->m_rbH264->m_semaFree->release();//空闲信号量加1.
                    if(nH264Len<=0)
                    {
                        qDebug()<<"<Error>:error get h264 frames.";
                        break;
                    }

                    //Audio Packet format: pkt len + pkt data.
                    QByteArray baH264PktLen=qint32ToQByteArray(nH264Len);
                    if(tcpSocket->write(baH264PktLen)<0)
                    {
                        qDebug()<<"<Error>:socket write error,break it.";
                        break;
                    }
                    if(tcpSocket->write(txBuffer,nH264Len)<0)
                    {
                        qDebug()<<"<Error>:socket write error,break it.";
                        break;
                    }
                    tcpSocket->waitForBytesWritten(1000);
                    //qDebug()<<"tx h264:"<<baH264Data.size();
                }
                //设置连接标志，这样编码器线程就会停止工作.
                switch(this->m_nTcpPort)
                {
                case TCP_PORT_VIDEO:
                    gGblPara.m_bVideoTcpConnected=false;
                    break;
                case TCP_PORT_VIDEO2:
                    gGblPara.m_bVideoTcpConnected2=false;
                    break;
                default:
                    break;
                }
            }
        }
        delete tcpServer;
    }
    qDebug()<<"<MainLoop>:VideoTxThread ends ["<<this->m_nTcpPort<<"].";
    //此处设置本线程退出标志.
    //同时设置全局请求退出标志，请求其他线程退出.
    gGblPara.m_bVideoTxThreadExitFlag=true;
    gGblPara.m_bGblRst2Exit=true;
    emit this->ZSigThreadFinished();
}
