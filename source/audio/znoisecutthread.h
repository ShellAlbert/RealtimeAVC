#ifndef ZNOISECUTTHREAD_H
#define ZNOISECUTTHREAD_H

#include <QThread>
#include <QQueue>
#include <QSemaphore>
class ZNoiseCutThread : public QThread
{
    Q_OBJECT
public:
    ZNoiseCutThread();

    qint32 ZBindWaveFormQueueBefore(QQueue<QByteArray> *queue,QSemaphore *semaUsed,QSemaphore *semaFree);
    qint32 ZBindWaveFormQueueAfter(QQueue<QByteArray> *queue,QSemaphore *semaUsed,QSemaphore *semaFree);
    qint32 ZStartThread(QQueue<QByteArray> *queueNoise,QSemaphore *semaUsedNoise,QSemaphore *semaFreeNoise,///<
                        QQueue<QByteArray> *queueClear,QSemaphore *semaUsedClear,QSemaphore *semaFreeClear,///<
                        QQueue<QByteArray> *queueEncode,QSemaphore *semaUsedEncode,QSemaphore *semaFreeEncode);
    qint32 ZStopThread();
signals:
    void ZSigThreadFinished();
protected:
    void run();
private:
    QQueue<QByteArray> *m_queueNoise;
    QSemaphore *m_semaUsedNoise;
    QSemaphore *m_semaFreeNoise;

    QQueue<QByteArray> *m_queueClear;
    QSemaphore *m_semaUsedClear;
    QSemaphore *m_semaFreeClear;

    QQueue<QByteArray> *m_queueEncode;
    QSemaphore *m_semaUsedEncode;
    QSemaphore *m_semaFreeEncode;

private:
    //波形显示队列，降噪算法处理之前与处理之后波形比对
     QQueue<QByteArray> *m_queueWavBefore;
     QSemaphore *m_semaUsedWavBefore;
     QSemaphore *m_semaFreeWavBefore;

     QQueue<QByteArray> *m_queueWavAfter;
     QSemaphore *m_semaUsedWavAfter;
     QSemaphore *m_semaFreeWavAfter;
};

#endif // ZNOISECUTTHREAD_H
