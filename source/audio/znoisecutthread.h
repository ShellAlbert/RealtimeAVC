#ifndef ZNOISECUTTHREAD_H
#define ZNOISECUTTHREAD_H

#include <QThread>
#include <QQueue>
#include <QSemaphore>
#include <zringbuffer.h>
class ZNoiseCutThread : public QThread
{
    Q_OBJECT
public:
    ZNoiseCutThread();
    qint32 ZBindWaveFormQueue(ZRingBuffer *rbWaveBefore,ZRingBuffer *rbWaveAfter);
    qint32 ZStartThread(ZRingBuffer *m_rbNoise,ZRingBuffer *m_rbPlay,ZRingBuffer *m_rbTx);
    qint32 ZStopThread();
    bool ZIsExitCleanup();
signals:
    void ZSigThreadFinished();
    void ZSigNewWaveBeforeArrived(const QByteArray &baPCM);
    void ZSigNewWaveAfterArrived(const QByteArray &baPCM);
protected:
    void run();
private:
    ZRingBuffer *m_rbNoise;
    ZRingBuffer *m_rbPlay;
    ZRingBuffer *m_rbTx;
private:
    //波形显示队列，降噪算法处理之前与处理之后波形比对
    ZRingBuffer *m_rbWaveBefore;
    ZRingBuffer *m_rbWaveAfter;
private:
    bool m_bCleanup;
};

#endif // ZNOISECUTTHREAD_H
