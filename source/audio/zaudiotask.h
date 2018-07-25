#ifndef ZAUDIOTASK_H
#define ZAUDIOTASK_H
#include "../zgblpara.h"
#include <QWidget>
#include <audio/zaudiocapturethread.h>
#include <audio/znoisecutthread.h>
#include <audio/zaudioplaythread.h>
#include <audio/zpcmencthread.h>
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
private slots:
    void ZSlotCheckExitFlag();
    void ZSlotTimeout();
private:
    ZAudioCaptureThread *m_capThread;
    ZNoiseCutThread *m_cutThread;
    ZAudioPlayThread *m_playThread;
    ZPCMEncThread *m_pcmEncThread;
    ZAudioTxThread *m_txThread;

    //CaptureThread capture pcm data and put then into NoiseQueue.
    ZRingBuffer *m_rbNoise;

    //NoiseCutThread get data from NoiseQueue and do noise cut then put them into ClearQueue&EncodeQueue.
    //PlayThread get data from ClearQueue and write pcm data to hardware.
    //EncodeThread get data from EncodeQueue and do OpusEncode and put them into TcpQueue.
    //TcpThread get data from TxQueue and send it out.
    ZRingBuffer *m_rbClear;
    ZRingBuffer *m_rbEncode;
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
