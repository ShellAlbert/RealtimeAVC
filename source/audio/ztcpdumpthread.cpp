#include "ztcpdumpthread.h"
#include "../zgblpara.h"
#include <opus/opus.h>
#include <opus/opus_multistream.h>
#include <opus/opus_defines.h>
#include <opus/opus_types.h>
#include <sys/types.h>
#include <sys/socket.h>

ZTcpDumpThread::ZTcpDumpThread()
{
    this->m_bExitFlag=false;
}
void ZTcpDumpThread::run()
{
    qDebug()<<"<MainLoop>:TcpDumpThread starts.";
    while(!gGblPara.m_bGblRst2Exit)
    {
        QTcpServer *tcpServer=new QTcpServer;
        int on=1;
        int sockFd=tcpServer->socketDescriptor();
        setsockopt(sockFd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
        if(!tcpServer->listen(QHostAddress::Any,TCP_PORT))
        {
            qDebug()<<"<error>: tcp server error listen on port"<<TCP_PORT;
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
                gGblPara.m_audio.m_bTcpAudioConnected=true;
                //qDebug()<<"audio connected.";

                //向客户端发送音频数据包.
                while(!gGblPara.m_bGblRst2Exit)
                {
                    //fetch data from tcp queue.
                    QByteArray baOpusData;
                    this->m_semaUsed->acquire();//已用信号量减1.
                    baOpusData=this->m_queue->dequeue();
                    this->m_semaFree->release();//空闲信号量加1.

                    //Audio Packet format: pkt len + pkt data.
                    QByteArray baOpusPktLen=qint32ToQByteArray(baOpusData.size());
                    //qDebug("%d:%02x %02x %02x %02x\n",baOpusData.size(),(uchar)baOpusPktLen.at(0),(uchar)baOpusPktLen.at(1),(uchar)baOpusPktLen.at(2),(uchar)baOpusPktLen.at(3));
                    if(tcpSocket->write(baOpusPktLen)<0)
                    {
                        qDebug()<<"<error>:socket write error,break it.";
                        break;
                    }
                    if(tcpSocket->write(baOpusData)<0)
                    {
                        qDebug()<<"<error>:socket write error,break it.";
                        break;
                    }
                    tcpSocket->waitForBytesWritten(1000);
                    this->usleep(AUDIO_THREAD_SCHEDULE_US);
                }
                //设置连接标志，这样编码器线程就会停止工作.
                gGblPara.m_audio.m_bTcpAudioConnected=false;
                //qDebug()<<"audio disconnected.";
            }
        }
        delete tcpServer;
    }
    qDebug()<<"<MainLoop>:TcpDumpThread ends.";
    emit this->ZSigThreadFinished();
}
qint32 ZTcpDumpThread::ZStartThread(QQueue<QByteArray> *queue,QSemaphore *semaUsed,QSemaphore *semaFree)
{
    this->m_queue=queue;
    this->m_semaUsed=semaUsed;
    this->m_semaFree=semaFree;

    this->start();
    return 0;
}
qint32 ZTcpDumpThread::ZStopThread()
{
    this->m_bExitFlag=true;
    return 0;
}
