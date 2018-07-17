#ifndef ZVIDEOTXTHREAD_H
#define ZVIDEOTXTHREAD_H

#include <QThread>
#include <QQueue>
#include <QSemaphore>
#include <QTcpServer>
#include <QTcpSocket>
class ZVideoTxThread : public QThread
{
    Q_OBJECT
public:
    ZVideoTxThread();
    qint32 ZBindQueue(QQueue<QByteArray> *queue,QSemaphore *semaUsed,QSemaphore *semaFree);
    qint32 ZStartThread();
    qint32 ZStopThread();
signals:
    void ZSigThreadFinished();

protected:
    void run();
private:
    QQueue<QByteArray> *m_queue;
    QSemaphore *m_semaUsed;
    QSemaphore *m_semaFree;
    bool m_bExitFlag;
};

#endif // ZVIDEOTXTHREAD_H
