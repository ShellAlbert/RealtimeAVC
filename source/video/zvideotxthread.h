#ifndef ZVIDEOTXTHREAD_H
#define ZVIDEOTXTHREAD_H

#include <QThread>
#include <QQueue>
#include <QSemaphore>
#include <QTcpServer>
#include <QTcpSocket>
#include <zringbuffer.h>
class ZVideoTxThread : public QThread
{
    Q_OBJECT
public:
    ZVideoTxThread();
    qint32 ZBindQueue(ZRingBuffer *rbH264);
    qint32 ZStartThread();
    qint32 ZStopThread();
signals:
    void ZSigThreadFinished();

protected:
    void run();
private:
    ZRingBuffer *m_rbH264;
    bool m_bExitFlag;
};

#endif // ZVIDEOTXTHREAD_H
