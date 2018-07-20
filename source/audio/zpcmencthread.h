#ifndef ZPCMENCTHREAD_H
#define ZPCMENCTHREAD_H

#include <QThread>
#include <QQueue>
#include <QSemaphore>
#include "../zgblpara.h"
#include <zringbuffer.h>

class ZPCMEncThread : public QThread
{
    Q_OBJECT
public:
    ZPCMEncThread();

    qint32 ZStartThread(ZRingBuffer *rbEncode,ZRingBuffer *rbTx);
    qint32 ZStopThread();
    bool ZIsRunning();
protected:
    void run();
signals:
    void ZSigThreadFinished();
private:
    ZRingBuffer *m_rbEncode;
    ZRingBuffer *m_rbTx;
    bool m_bRunning;
};

#endif // ZPCMENCTHREAD_H
