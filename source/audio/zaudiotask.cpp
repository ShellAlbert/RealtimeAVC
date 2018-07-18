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
    this->m_queueNoise=NULL;
    this->m_semaUsedNoise=NULL;
    this->m_semaFreeNoise=NULL;

    //clear queue.
    //noise cut thread -> play thread.
    this->m_queueClear=NULL;
    this->m_semaUsedClear=NULL;
    this->m_semaFreeClear=NULL;

    //encode queue.
    //noise cut thread -> encode thread.
    this->m_queueEncode=NULL;
    this->m_semaUsedEncode=NULL;
    this->m_semaFreeEncode=NULL;

    this->m_queueTCP=NULL;
    this->m_semaUsedTCP=NULL;
    this->m_semaFreeTCP=NULL;

    //capture-process-playback mode.
    this->m_capThread=NULL;
    this->m_cutThread=NULL;
    this->m_playThread=NULL;
    this->m_pcmEncThread=NULL;
    this->m_tcpThread=NULL;
}

ZAudioTask::~ZAudioTask()
{
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

    //noise queue.
    if(this->m_queueNoise!=NULL)
    {
        delete this->m_queueNoise;
    }
    if(this->m_semaUsedNoise!=NULL)
    {
        delete this->m_semaUsedNoise;
    }
    if(this->m_semaFreeNoise!=NULL)
    {
        delete this->m_semaFreeNoise;
    }

    //clear queue.
    if(this->m_queueClear!=NULL)
    {
        delete this->m_queueClear;
    }
    if(this->m_semaUsedClear!=NULL)
    {
        delete this->m_semaUsedClear;
    }
    if(this->m_semaFreeClear!=NULL)
    {
        delete this->m_semaFreeClear;
    }


    //encode queue.
    if(this->m_queueEncode!=NULL)
    {
        this->m_queueEncode=NULL;
    }
    if(this->m_semaUsedEncode!=NULL)
    {
        delete this->m_semaUsedEncode;
    }
    if(this->m_semaFreeEncode!=NULL)
    {
        delete this->m_semaFreeEncode;
    }
}
qint32 ZAudioTask::ZBindWaveFormQueueBefore(QQueue<QByteArray> *queue,QSemaphore *semaUsed,QSemaphore *semaFree)
{
    this->m_queueWavBefore=queue;
    this->m_semaUsedWavBefore=semaUsed;
    this->m_semaFreeWavBefore=semaFree;
    return 0;
}
qint32 ZAudioTask::ZBindWaveFormQueueAfter(QQueue<QByteArray> *queue,QSemaphore *semaUsed,QSemaphore *semaFree)
{
    this->m_queueWavAfter=queue;
    this->m_semaUsedWavAfter=semaUsed;
    this->m_semaFreeWavAfter=semaFree;
    return 0;
}
qint32 ZAudioTask::ZStartTask()
{
    this->m_timer=new QTimer;
    QObject::connect(this->m_timer,SIGNAL(timeout()),this,SLOT(ZSlotTimeout()));
    this->m_timer->start(5000);

    //noise queue.
    //capture thread -> noise cut thread.
    this->m_queueNoise=new QQueue<QByteArray>;
    this->m_semaUsedNoise=new QSemaphore(0);
    this->m_semaFreeNoise=new QSemaphore(30);

    //clear queue.
    //noise cut thread -> play thread.
    this->m_queueClear=new QQueue<QByteArray>;
    this->m_semaUsedClear=new QSemaphore(0);
    this->m_semaFreeClear=new QSemaphore(30);

    //encode queue.
    //noise cut thread -> encode thread.
    this->m_queueEncode=new QQueue<QByteArray>;
    this->m_semaUsedEncode=new QSemaphore(0);
    this->m_semaFreeEncode=new QSemaphore(30);

    this->m_queueTCP=new QQueue<QByteArray>;
    this->m_semaUsedTCP=new QSemaphore(0);
    this->m_semaFreeTCP=new QSemaphore(30);


    //CaptureThread -> queueNoise -> NoiseCutThread  -> queueEncode -> PcmEncThread -> queueTCP -> TcpDumpThread.
    //                                               -> queueClear  -> PlaybackThread.


    //create capture thread.
    this->m_capThread=new ZAudioCaptureThread(gGblPara.m_audio.m_capCardName,false);
    QObject::connect(this->m_capThread,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotCheckExitFlag()));

    //create noise cut thread.
    this->m_cutThread=new ZNoiseCutThread;
    this->m_cutThread->ZBindWaveFormQueueBefore(this->m_queueWavBefore,this->m_semaUsedWavBefore,this->m_semaFreeWavBefore);
    this->m_cutThread->ZBindWaveFormQueueAfter(this->m_queueWavAfter,this->m_semaUsedWavAfter,this->m_semaFreeWavAfter);
    QObject::connect(this->m_cutThread,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotCheckExitFlag()));

    //create playback thread.
    this->m_playThread=new ZAudioPlayThread(gGblPara.m_audio.m_playCardName);
    QObject::connect(this->m_playThread,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotCheckExitFlag()));

    //pcm encode thread.
    this->m_pcmEncThread=new ZPCMEncThread;
    connect(this->m_pcmEncThread,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotCheckExitFlag()));

    //tcp dump thread.
    this->m_tcpThread=new ZTcpDumpThread;
    QObject::connect(this->m_tcpThread,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotCheckExitFlag()));

    //start thread.
    this->m_capThread->ZStartThread(this->m_queueNoise,this->m_semaUsedNoise,this->m_semaFreeNoise);
    this->m_cutThread->ZStartThread(this->m_queueNoise,this->m_semaUsedNoise,this->m_semaFreeNoise,///<
                                    this->m_queueClear,this->m_semaUsedClear,this->m_semaFreeClear,///<
                                    this->m_queueEncode,this->m_semaUsedEncode,this->m_semaFreeEncode);

    this->m_playThread->ZStartThread(this->m_queueClear,this->m_semaUsedClear,this->m_semaFreeClear);
    //////////////////////////////////////////////////////////////////////////////////////////////////
    this->m_pcmEncThread->ZStartThread(this->m_queueEncode,this->m_semaUsedEncode,this->m_semaFreeEncode,///<
                                       this->m_queueTCP,this->m_semaUsedTCP,this->m_semaFreeTCP);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    this->m_tcpThread->ZStartThread(this->m_queueTCP,this->m_semaUsedTCP,this->m_semaFreeTCP);
    return 0;
}
void ZAudioTask::ZSlotCheckExitFlag()
{

    if(gGblPara.m_audio.m_bCapThreadExitFlag && gGblPara.m_audio.m_bCutThreadExitFlag && ///<
            gGblPara.m_audio.m_bPlayThreadExitFlag && gGblPara.m_audio.m_bPCMEncThreadExitFlag && ///<
            gGblPara.m_audio.m_bTcpDumpThreadExitFlag)
    {
        this->m_capThread->quit();
        this->m_capThread->wait(1000);

        this->m_cutThread->quit();
        this->m_cutThread->wait(1000);

        this->m_playThread->quit();
        this->m_playThread->wait(1000);

        this->m_pcmEncThread->quit();
        this->m_pcmEncThread->wait(1000);

        this->m_tcpThread->quit();
        this->m_tcpThread->wait(1000);

        qApp->exit(0);
    }
}
void ZAudioTask::ZSlotTimeout()
{
    if(gGblPara.m_audio.m_nCapOverrun!=this->m_nCapOverrun || gGblPara.m_audio.m_nPlayUnderrun!=this->m_nPlayUnderrun)
    {
        this->m_nCapOverrun=gGblPara.m_audio.m_nCapOverrun;
        this->m_nPlayUnderrun=gGblPara.m_audio.m_nPlayUnderrun;
        qDebug("<Warning>: Capture Overrun:%d(queue:%d),Playback Underrun:%d(queue:%d).",this->m_nCapOverrun,this->m_queueNoise->size(),///<
               this->m_nPlayUnderrun,this->m_queueClear->size());
    }
}
