#ifndef ZAUDIOTASK_H
#define ZAUDIOTASK_H
#include "../zgblpara.h"
#include <QWidget>
#include <audio/zaudiocapturethread.h>
#include <audio/znoisecutthread.h>
#include <audio/zaudioplaythread.h>
//#include <audio/zpcmencthread.h>
#include <audio/zaudiotxthread.h>
#include <QSemaphore>
#include <QQueue>
#include <QTimer>
#include <zringbuffer.h>

class ZAudioTask : public QObject
{
    Q_OBJECT
public:
     ZAudioTask(QObject *parent = 0);
    ~ZAudioTask();
     qint32 ZBindWaveFormQueueBefore(ZRingBuffer *rbWave);
     qint32 ZBindWaveFormQueueAfter(ZRingBuffer *rbWave);
     qint32 ZStartTask();

     ZNoiseCutThread* ZGetNoiseCutThread();
     bool ZIsExitCleanup();
signals:
     void ZSigAudioTaskExited();
private slots:
    void ZSlotCheckExitFlag();
    void ZSlotTimeout();
private:
    //Audio Capture --noise queue-->  Noise Cut --play queue--> Local Play.
    //                                          -- tx queue --> Tcp Tx.
    ZAudioCaptureThread *m_capThread;
    ZNoiseCutThread *m_cutThread;
    ZAudioPlayThread *m_playThread;
    ZAudioTxThread *m_txThread;

    //ringBuffer.
    ZRingBuffer *m_rbNoise;
    ZRingBuffer *m_rbPlay;
    ZRingBuffer *m_rbTx;
private:
    QTimer *m_timer;
    qint32 m_nCapOverrun;
    qint32 m_nPlayUnderrun;
private:
   //波形显示队列，降噪算法处理之前与处理之后波形比对
    ZRingBuffer *m_rbWaveBefore;
    ZRingBuffer *m_rbWaveAfter;
};

#endif // ZAUDIOTASK_H
