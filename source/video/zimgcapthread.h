#ifndef ZIMGCAPTHREAD_H
#define ZIMGCAPTHREAD_H

#include <QThread>
#include <QtGui/QImage>
#include "zcamdevice.h"
#include "zgblpara.h"
#include <QTimer>
#include <QQueue>
#include <QByteArray>
#include <QSemaphore>
#include <zringbuffer.h>
class ZImgCapThread : public QThread
{
    Q_OBJECT
public:
    explicit ZImgCapThread(QString devNodeName,qint32 nPreWidth,qint32 nPreHeight,qint32 nPreFps,bool bMainCamera=false);
    ~ZImgCapThread();

    qint32 ZBindProcessQueue(ZRingBuffer *rbProcess);
    qint32 ZBindYUVQueue(ZRingBuffer *rbYUV);

    qint32 ZStartThread();
    qint32 ZStopThread();

    qint32 ZGetCAMImgFps();

    QString ZGetDevName();

    bool ZIsExitCleanup();
signals:
    void ZSigNewImgArrived(QImage img);
    void ZSigMsg(const QString &msg,const qint32 &type);
    void ZSigThreadFinished();
    void ZSigCAMIDFind(QString camID);
protected:
    void run();
private:
    QString m_devName;
    qint32 m_nPreWidth,m_nPreHeight,m_nPreFps;
    ZCAMDevice *m_cam;
private:
    //capture image to process queue.
    ZRingBuffer *m_rbProcess;
    //capture yuv to yuv queue.
    ZRingBuffer *m_rbYUV;

    bool m_bMainCamera;
private:
    bool m_bExitFlag;
    bool m_bCleanup;
};
#endif // ZIMGCAPTHREAD_H
