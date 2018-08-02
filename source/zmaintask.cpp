#include "zmaintask.h"

ZMainTask::ZMainTask()
{
    this->m_tcp2Uart=NULL;
    this->m_ctlJson=NULL;
    this->m_audio=NULL;
    this->m_video=NULL;
    this->m_ui=NULL;
}
ZMainTask::~ZMainTask()
{
    delete this->m_timerExit;

    delete this->m_tcp2Uart;
    delete this->m_ctlJson;
    delete this->m_audio;
    delete this->m_video;
    delete this->m_ui;
}
qint32 ZMainTask::ZStartTask()
{
    this->m_timerExit=new QTimer;
    QObject::connect(this->m_timerExit,SIGNAL(timeout()),this,SLOT(ZSlotChkAllExitFlags()));

    //start Android(tcp) <--> STM32(uart) forward task.
    this->m_tcp2Uart=new ZTcp2UartForwardThread;
    QObject::connect(this->m_tcp2Uart,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotSubThreadsExited()));
    this->m_tcp2Uart->ZStartThread();

    //ctl thread for Video/Audio.
    this->m_ctlJson=new ZCtlServer;
    //QObject::connect(this->m_ctlJson,SIGNAL(ZSigThreadExited()),this,SLOT(ZSlotSubThreadsExited()));
    this->m_ctlJson->ZStartServer();

    //audio task.
    this->m_audio=new ZAudioTask;
    QObject::connect(this->m_audio,SIGNAL(ZSigAudioTaskExited()),this,SLOT(ZSlotSubThreadsExited()));
    if(this->m_audio->ZStartTask()<0)
    {
        qDebug()<<"<Error>:failed to start audio task.";
        return -1;
    }

    //video task.
    this->m_video=new ZVideoTask;
    QObject::connect(this->m_video,SIGNAL(ZSigVideoTaskExited()),this,SLOT(ZSlotSubThreadsExited()));
    if(this->m_video->ZDoInit()<0)
    {
        qDebug()<<"<Error>:failed to init video task.";
        return -1;
    }
    if(this->m_video->ZStartTask()<0)
    {
        qDebug()<<"<Error>:failed to start video task!";
        return -1;
    }

    //AV UI.
    this->m_ui=new ZAVUI;

    //use signal-slot event to notify local UI flush.
    QObject::connect(this->m_video->ZGetImgCapThread(0),SIGNAL(ZSigNewImgArrived(QImage)),this->m_ui->ZGetImgDisp(0),SLOT(ZSlotFlushImg(QImage)),Qt::AutoConnection);
    QObject::connect(this->m_video->ZGetImgCapThread(1),SIGNAL(ZSigNewImgArrived(QImage)),this->m_ui->ZGetImgDisp(1),SLOT(ZSlotFlushImg(QImage)),Qt::AutoConnection);

    //use signal-slot event to notify UI to flush new image process set.
    QObject::connect(this->m_video->ZGetImgProcessThread(),SIGNAL(ZSigNewProcessSetArrived(ZImgProcessedSet)),this->m_ui,SLOT(ZSlotFlushProcessedSet(ZImgProcessedSet)),Qt::AutoConnection);
    QObject::connect(this->m_video->ZGetImgProcessThread(),SIGNAL(ZSigSSIMImgSimilarity(qint32)),this->m_ui,SLOT(ZSlotSSIMImgSimilarity(qint32)),Qt::AutoConnection);

    //use signal-slot event to flush wave form.
    QObject::connect(this->m_audio->ZGetNoiseCutThread(),SIGNAL(ZSigNewWaveBeforeArrived(QByteArray)),this->m_ui,SLOT(ZSlotFlushWaveBefore(QByteArray)),Qt::AutoConnection);
    QObject::connect(this->m_audio->ZGetNoiseCutThread(),SIGNAL(ZSigNewWaveAfterArrived(QByteArray)),this->m_ui,SLOT(ZSlotFlushWaveAfter(QByteArray)),Qt::AutoConnection);

    this->m_ui->showMaximized();

    return 0;
}
void ZMainTask::ZSlotSubThreadsExited()
{
    if(!this->m_timerExit->isActive())
    {
        this->m_timerExit->start(1000);
    }
}
void ZMainTask::ZSlotChkAllExitFlags()
{
    if(!this->m_tcp2Uart->ZIsExitCleanup())
    {
        qDebug()<<"<Exit>:wait for tcp2uart thread.";
        return;
    }
//    if(!this->m_ctlJson->ZIsExitCleanup())
//    {
//        qDebug()<<"<Exit>:wait for ctlJson thread.";
//        return;
//    }
    if(!this->m_audio->ZIsExitCleanup())
    {
        qDebug()<<"<Exit>:wait for audio task.";
        return;
    }
    if(!this->m_video->ZIsExitCleanup())
    {
        qDebug()<<"<Exit>:wait for m_video task.";
        return;
    }

    this->m_timerExit->stop();
    qApp->exit(0);
}
void ZMainTask::ZSlotFwdImgProcessedSet2Ctl(const ZImgProcessedSet &set)
{
    if(this->m_vecImgMatched.size()>50)
    {
        this->m_vecImgMatched.removeFirst();
    }
    this->m_vecImgMatched.append(set);
}
