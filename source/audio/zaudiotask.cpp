#include "zaudiotask.h"
#include "../zgblpara.h"
#include <QApplication>
ZAudioTask::ZAudioTask(QObject *parent):QObject(parent)
{
    this->m_nCapOverrun=0;
    this->m_nPlayUnderrun=0;

    this->m_timer=NULL;

    //noise queue.
    //capture thread -> noise cut thread.
    this->m_rbNoise=NULL;

    //clear queue.
    //noise cut thread -> play thread.
    this->m_rbClear=NULL;

    //encode queue.
    //noise cut thread -> encode thread.
    this->m_rbEncode=NULL;

    //tx queue.
    //h264 encode thread -> tcp tx thread.
    this->m_rbTx=NULL;

    //capture-process-playback mode.
    this->m_capThread=NULL;
    this->m_cutThread=NULL;
    this->m_playThread=NULL;
    this->m_pcmEncThread=NULL;
    this->m_txThread=NULL;
}

ZAudioTask::~ZAudioTask()
{
    this->m_timer->stop();
    delete this->m_timer;

    //capture-process-playback mode.
    if(this->m_capThread!=NULL)
    {
        delete this->m_capThread;
    }
    if(this->m_cutThread!=NULL)
    {
        delete this->m_cutThread;
    }
    if(this->m_playThread!=NULL)
    {
        delete this->m_playThread;
    }
    if(this->m_pcmEncThread!=NULL)
    {
        delete this->m_pcmEncThread;
    }
    if(this->m_txThread!=NULL)
    {
        delete this->m_txThread;
    }
}
qint32 ZAudioTask::ZBindWaveFormQueueBefore(ZRingBuffer *rbWave)
{
    this->m_rbWaveBefore=rbWave;
    return 0;
}
qint32 ZAudioTask::ZBindWaveFormQueueAfter(ZRingBuffer *rbWave)
{
    this->m_rbWaveAfter=rbWave;
    return 0;
}
qint32 ZAudioTask::ZStartTask()
{
    this->m_timer=new QTimer;
    QObject::connect(this->m_timer,SIGNAL(timeout()),this,SLOT(ZSlotTimeout()));
    this->m_timer->start(5000);

    //noise queue.
    //capture thread -> noise cut thread.
    this->m_rbNoise=new ZRingBuffer(30,BLOCK_SIZE);

    //clear queue.
    //noise cut thread -> play thread.
    this->m_rbClear=new ZRingBuffer(30,BLOCK_SIZE);

    //encode queue.
    //noise cut thread -> encode thread.
    this->m_rbEncode=new ZRingBuffer(30,BLOCK_SIZE);

    //tx queue.
    //h264 encode thread -> tcp tx thread.
    this->m_rbTx=new ZRingBuffer(30,BLOCK_SIZE);

    //CaptureThread -> queueNoise -> NoiseCutThread  -> queueEncode -> PcmEncThread -> queueTCP -> TcpDumpThread.
    //                                               -> queueClear  -> PlaybackThread.


    //create capture thread.
    this->m_capThread=new ZAudioCaptureThread(gGblPara.m_audio.m_capCardName,false);
    QObject::connect(this->m_capThread,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotCheckExitFlag()));

    //create noise cut thread.
    this->m_cutThread=new ZNoiseCutThread;
    this->m_cutThread->ZBindWaveFormQueueBefore(this->m_rbWaveBefore);
    this->m_cutThread->ZBindWaveFormQueueAfter(this->m_rbWaveAfter);
    QObject::connect(this->m_cutThread,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotCheckExitFlag()));

    //create playback thread.
    this->m_playThread=new ZAudioPlayThread(gGblPara.m_audio.m_playCardName);
    QObject::connect(this->m_playThread,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotCheckExitFlag()));

    //pcm encode thread.
    this->m_pcmEncThread=new ZPCMEncThread;
    connect(this->m_pcmEncThread,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotCheckExitFlag()));

    //tcp tx thread.
    this->m_txThread=new ZAudioTxThread;
    QObject::connect(this->m_txThread,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotCheckExitFlag()));

    //start thread.
    this->m_capThread->ZStartThread(this->m_rbNoise);
    this->m_cutThread->ZStartThread(this->m_rbNoise,this->m_rbClear,this->m_rbEncode);
    this->m_playThread->ZStartThread(this->m_rbClear);
    //////////////////////////////////////////////////////////////////////////////////////////////////
    this->m_pcmEncThread->ZStartThread(this->m_rbEncode,this->m_rbTx);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    this->m_txThread->ZStartThread(this->m_rbTx);
    return 0;
}
ZNoiseCutThread* ZAudioTask::ZGetNoiseCutThread()
{
    return this->m_cutThread;
}
void ZAudioTask::ZSlotCheckExitFlag()
{

    if(gGblPara.m_audio.m_bCapThreadExitFlag && gGblPara.m_audio.m_bCutThreadExitFlag && ///<
            gGblPara.m_audio.m_bPlayThreadExitFlag && gGblPara.m_audio.m_bPCMEncThreadExitFlag && ///<
            gGblPara.m_audio.m_bTcpTxThreadExitFlag)
    {
        this->m_capThread->quit();
        this->m_capThread->wait(1000);

        this->m_cutThread->quit();
        this->m_cutThread->wait(1000);

        this->m_playThread->quit();
        this->m_playThread->wait(1000);

        this->m_pcmEncThread->quit();
        this->m_pcmEncThread->wait(1000);

        this->m_txThread->quit();
        this->m_txThread->wait(1000);
    }
}
void ZAudioTask::ZSlotTimeout()
{
    if(gGblPara.m_audio.m_nCapOverrun!=this->m_nCapOverrun || gGblPara.m_audio.m_nPlayUnderrun!=this->m_nPlayUnderrun)
    {
        this->m_nCapOverrun=gGblPara.m_audio.m_nCapOverrun;
        this->m_nPlayUnderrun=gGblPara.m_audio.m_nPlayUnderrun;
        qDebug("<Warning>: Capture Overrun:%d(queue:%d),Playback Underrun:%d(queue:%d).",///<
               this->m_nCapOverrun,this->m_rbNoise->ZGetValidNum(),///<
               this->m_nPlayUnderrun,this->m_rbClear->ZGetValidNum());
    }
}
