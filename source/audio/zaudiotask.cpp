#include "zaudiotask.h"
#include "../zgblpara.h"
#include <QApplication>
ZAudioTask::ZAudioTask(QObject *parent):QObject(parent)
{
    this->m_nCapOverrun=0;
    this->m_nPlayUnderrun=0;

    this->m_timer=NULL;

    this->m_rbNoise=NULL;
    this->m_rbPlay=NULL;
    this->m_rbTx=NULL;

    //capture-process-playback/tcp tx mode.
    this->m_capThread=NULL;
    this->m_cutThread=NULL;
    this->m_playThread=NULL;
    this->m_txThread=NULL;
}

ZAudioTask::~ZAudioTask()
{
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

    //noise queue for capture thread -> noise cut thread.
    this->m_rbNoise=new ZRingBuffer(MAX_AUDIO_RING_BUFFER,BLOCK_SIZE);

    //play queue for noise cut thread -> play thread.
    this->m_rbPlay=new ZRingBuffer(MAX_AUDIO_RING_BUFFER,BLOCK_SIZE);

    //tx queue for noise cut thread -> tcp tx thread.
    this->m_rbTx=new ZRingBuffer(MAX_AUDIO_RING_BUFFER,BLOCK_SIZE);

    //Audio Capture --noise queue-->  Noise Cut --play queue--> Local Play.
    //                                          -- tx queue --> Tcp Tx.
    this->m_capThread=new ZAudioCaptureThread(gGblPara.m_audio.m_capCardName,false);
    QObject::connect(this->m_capThread,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotCheckExitFlag()));

    //create noise cut thread.
    this->m_cutThread=new ZNoiseCutThread;
    this->m_cutThread->ZBindWaveFormQueue(this->m_rbWaveBefore,this->m_rbWaveAfter);
    QObject::connect(this->m_cutThread,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotCheckExitFlag()));

    //create playback thread.
    this->m_playThread=new ZAudioPlayThread(gGblPara.m_audio.m_playCardName);
    QObject::connect(this->m_playThread,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotCheckExitFlag()));

    //tcp tx thread.
    this->m_txThread=new ZAudioTxThread;
    QObject::connect(this->m_txThread,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotCheckExitFlag()));

    //start thread.
    this->m_capThread->ZStartThread(this->m_rbNoise);
    this->m_cutThread->ZStartThread(this->m_rbNoise,this->m_rbPlay,this->m_rbTx);
    this->m_playThread->ZStartThread(this->m_rbPlay);
    this->m_txThread->ZStartThread(this->m_rbTx);
    return 0;
}
ZNoiseCutThread* ZAudioTask::ZGetNoiseCutThread()
{
    return this->m_cutThread;
}
void ZAudioTask::ZSlotCheckExitFlag()
{
    if(!this->m_timer->isActive())
    {
        this->m_timer->start(1000);
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
               this->m_nPlayUnderrun,this->m_rbPlay->ZGetValidNum());
    }
    //如果检测到全局请求退出标志，则清空所有队列，唤醒所有子线程。
    if(gGblPara.m_bGblRst2Exit)
    {
        //如果CapThread没有退出，可能noise queue队列满,则模拟NoiseCut线程出队一个音频帧，解除阻塞.
        if(!this->m_capThread->ZIsExitCleanup())
        {
            qDebug()<<"wait for audio cap thread";
            if(this->m_rbNoise->ZGetValidNum()==MAX_AUDIO_RING_BUFFER)
            {
                this->m_rbNoise->m_semaUsed->acquire();
                //this->m_rbNoise->ZGetElement(NULL,0);//出列数据无用，所以这里并不真取。
                this->m_rbNoise->m_semaFree->release();
            }
        }

        //如果NoiseCut没有退出，可能noise queue队列空，则模拟CapThread入队一个静音数据，解除阻塞。
        //也有可能是play queue满造成的阻塞，则模拟play thread取出一帧数据，解除阻塞。
        if(!this->m_cutThread->ZIsExitCleanup())
        {
            qDebug()<<"wait for audio cut thread";
            if(this->m_rbNoise->ZGetValidNum()==0)
            {
                qint8 *pMuteData=new qint8[BLOCK_SIZE];
                memset(pMuteData,0,BLOCK_SIZE);

                this->m_rbNoise->m_semaFree->acquire();
                this->m_rbNoise->ZPutElement(pMuteData,BLOCK_SIZE);
                this->m_rbNoise->m_semaUsed->release();

                delete  [] pMuteData;
            }
            if(this->m_rbPlay->ZGetValidNum()==MAX_AUDIO_RING_BUFFER)
            {
                this->m_rbPlay->m_semaUsed->acquire();
                //this->m_rbPlay->ZGetElement(NULL,0);//出列数据无用，所以这里并不真取。
                this->m_rbPlay->m_semaFree->release();
            }
        }

        //如果play thread没有退出，可能play queue队列空，则模拟noise cut投入一个静音数据，解除阻塞。
        if(!this->m_playThread->ZIsExitCleanup())
        {
            qDebug()<<"wait for audio play thread";
            if(this->m_rbPlay->ZGetValidNum()==0)
            {
                qint8 *pMuteData=new qint8[BLOCK_SIZE];
                memset(pMuteData,0,BLOCK_SIZE);

                this->m_rbPlay->m_semaFree->acquire();
                this->m_rbPlay->ZPutElement(pMuteData,BLOCK_SIZE);
                this->m_rbPlay->m_semaUsed->release();

                delete  [] pMuteData;
            }
        }

        //如果TxThread没有退出，可能tx queue队列空，则模拟noise cut投入一个静音数据，解除阻塞。
        if(!this->m_txThread->ZIsExitCleanup())
        {
            qDebug()<<"wait for audio tx thread";
            if(this->m_rbTx->ZGetValidNum()==0)
            {
                qint8 *pMuteData=new qint8[BLOCK_SIZE];
                memset(pMuteData,0,BLOCK_SIZE);

                this->m_rbTx->m_semaFree->acquire();
                this->m_rbTx->ZPutElement(pMuteData,BLOCK_SIZE);
                this->m_rbTx->m_semaUsed->release();

                delete  [] pMuteData;
            }
        }

        //当所有的子线程都退出时，则音频任务退出。
        if(this->ZIsExitCleanup())
        {
            this->m_timer->stop();
            emit this->ZSigAudioTaskExited();
        }
    }
}
bool ZAudioTask::ZIsExitCleanup()
{
    bool bAllCleanup=true;
    if(!this->m_capThread->ZIsExitCleanup())
    {
        bAllCleanup=false;
    }
    if(!this->m_cutThread->ZIsExitCleanup())
    {
        bAllCleanup=false;
    }
    if(!this->m_playThread->ZIsExitCleanup())
    {
        bAllCleanup=false;
    }
    if(!this->m_txThread->ZIsExitCleanup())
    {
        bAllCleanup=false;
    }
    return bAllCleanup;
}
