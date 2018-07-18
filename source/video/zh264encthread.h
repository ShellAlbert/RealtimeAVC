#ifndef ZH264ENCTHREAD_H
#define ZH264ENCTHREAD_H

#include <QThread>
#include <QSemaphore>
#include<QQueue>
class ZH264EncThread : public QThread
{
    Q_OBJECT
public:
    ZH264EncThread();
    qint32 ZBindYUVQueue(QQueue<QByteArray> *queueYUV,QSemaphore *semaYUVUsed,QSemaphore *semaYUVFree);
    qint32 ZBindH264Queue(QQueue<QByteArray> *queueH264,QSemaphore *semaH264Used,QSemaphore *semaH264Free);

    qint32 ZStartThread();
    qint32 ZStopThread();
protected:
    void run();
private:
    QQueue<QByteArray> *m_queueYUV;
    QSemaphore *m_semaYUVUsed;
    QSemaphore *m_semaYUVFree;

    QQueue<QByteArray> *m_queueH264;
    QSemaphore *m_semaH264Used;
    QSemaphore *m_semaH264Free;
private:
    bool m_bExitFlag;
};

#endif // ZH264ENCTHREAD_H
