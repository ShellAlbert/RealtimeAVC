#ifndef ZAUDIOTXTHREAD_H
#define ZAUDIOTXTHREAD_H

#include <QTcpSocket>
#include <QTcpServer>
#include <QThread>
#include <QMap>
#include <QQueue>
#include <QSemaphore>
#include <zringbuffer.h>
class ZAudioTxThread:public QThread
{
    Q_OBJECT
public:
    ZAudioTxThread();
    qint32 ZStartThread(ZRingBuffer *rbTx);
    qint32 ZStopThread();
signals:
    void ZSigThreadFinished();
protected:
    void run();
private:
    ZRingBuffer *m_rbTx;
    bool m_bExitFlag;
};
#endif // ZAUDIOTXTHREAD_H
