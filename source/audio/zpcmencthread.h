#ifndef ZPCMENCTHREAD_H
#define ZPCMENCTHREAD_H

#include <QThread>
#include <QQueue>
#include <QSemaphore>
#include "../zgblpara.h"


class ZPCMEncThread : public QThread
{
    Q_OBJECT
public:
    ZPCMEncThread();

    qint32 ZStartThread(QQueue<QByteArray> *queueEnc,QSemaphore *semaUsedEnc,QSemaphore *semaFreeEnc,///<
                        QQueue<QByteArray> *queueTCP,QSemaphore *semaUsedTCP,QSemaphore *semaFreeTCP);
    qint32 ZStopThread();
    bool ZIsRunning();
protected:
    void run();
signals:
    void ZSigThreadFinished();
private:
    QQueue<QByteArray> *m_queueEnc;
    QSemaphore *m_semaUsedEnc;
    QSemaphore *m_semaFreeEnc;
    QQueue<QByteArray> *m_queueTCP;
    QSemaphore *m_semaUsedTCP;
    QSemaphore *m_semaFreeTCP;
    bool m_bRunning;
};

#endif // ZPCMENCTHREAD_H
