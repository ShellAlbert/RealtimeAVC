#ifndef ZAUDIOTASK_H
#define ZAUDIOTASK_H
#include "../zgblpara.h"
#include <QWidget>
#include <audio/zaudiocapturethread.h>
#include <audio/znoisecutthread.h>
#include <audio/zaudioplaythread.h>
#include <audio/zpcmencthread.h>
#include <audio/ztcpdumpthread.h>
#include <QSemaphore>
#include <QQueue>
#include <QTimer>

class ZAudioTask : public QObject
{
    Q_OBJECT
public:
     ZAudioTask(QObject *parent = 0);
    ~ZAudioTask();

     qint32 ZBindWaveFormQueueBefore(QQueue<QByteArray> *queue,QSemaphore *semaUsed,QSemaphore *semaFree);
     qint32 ZBindWaveFormQueueAfter(QQueue<QByteArray> *queue,QSemaphore *semaUsed,QSemaphore *semaFree);
     qint32 ZStartTask();
private slots:
    void ZSlotCheckExitFlag();
    void ZSlotTimeout();
private:
    ZAudioCaptureThread *m_capThread;
    ZNoiseCutThread *m_cutThread;
    ZAudioPlayThread *m_playThread;
    ZPCMEncThread *m_pcmEncThread;
    ZTcpDumpThread *m_tcpThread;

    //CaptureThread capture pcm data and put then into NoiseQueue.
    QQueue<QByteArray> *m_queueNoise;
    QSemaphore *m_semaUsedNoise;
    QSemaphore *m_semaFreeNoise;

    //NoiseCutThread get data from NoiseQueue and do noise cut then put them into ClearQueue&EncodeQueue.
    //PlayThread get data from ClearQueue and write pcm data to hardware.
    //EncodeThread get data from EncodeQueue and do OpusEncode and put them into TcpQueue.
    //TcpThread get data from TcpQueue and send it out.
    QQueue<QByteArray> *m_queueClear;
    QSemaphore *m_semaUsedClear;
    QSemaphore *m_semaFreeClear;

    QQueue<QByteArray> *m_queueEncode;
    QSemaphore *m_semaUsedEncode;
    QSemaphore *m_semaFreeEncode;

    QQueue<QByteArray> *m_queueTCP;
    QSemaphore *m_semaUsedTCP;
    QSemaphore *m_semaFreeTCP;
private:
    QTimer *m_timer;
    qint32 m_nCapOverrun;
    qint32 m_nPlayUnderrun;


private:
   //波形显示队列，降噪算法处理之前与处理之后波形比对
    QQueue<QByteArray> *m_queueWavBefore;
    QSemaphore *m_semaUsedWavBefore;
    QSemaphore *m_semaFreeWavBefore;

    QQueue<QByteArray> *m_queueWavAfter;
    QSemaphore *m_semaUsedWavAfter;
    QSemaphore *m_semaFreeWavAfter;
};

#endif // ZAUDIOTASK_H
