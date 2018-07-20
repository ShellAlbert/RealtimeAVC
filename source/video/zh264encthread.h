#ifndef ZH264ENCTHREAD_H
#define ZH264ENCTHREAD_H

#include <QThread>
#include <QSemaphore>
#include<QQueue>
#include <zringbuffer.h>
class ZH264EncThread : public QThread
{
    Q_OBJECT
public:
    ZH264EncThread();
    qint32 ZBindYUVQueue(ZRingBuffer *rbYUV);
    qint32 ZBindH264Queue(ZRingBuffer *rbH264);

    qint32 ZStartThread();
    qint32 ZStopThread();
protected:
    void run();
private:
    ZRingBuffer *m_rbYUV;
    ZRingBuffer *m_rbH264;
private:
    bool m_bExitFlag;
};

#endif // ZH264ENCTHREAD_H
