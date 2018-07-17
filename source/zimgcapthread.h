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
class ZImgCapThread : public QThread
{
    Q_OBJECT
public:
    explicit ZImgCapThread(QString devNodeName,qint32 nPreWidth,qint32 nPreHeight,qint32 nPreFps,bool bMainCamera=false);
    ~ZImgCapThread();

    qint32 ZBindDispQueue(QQueue<QImage> *queueDisp,QSemaphore *semaDispUsed,QSemaphore *semaDispFree);
    qint32 ZBindProcessQueue(QQueue<QImage> *queueProcess,QSemaphore *semaProcessUsed,QSemaphore *semaProcessFree);
    qint32 ZBindYUVQueue(QQueue<QByteArray> *queueYUV,QSemaphore *semaYUVUsed,QSemaphore *semaYUVFree);

    qint32 ZStartThread();
    qint32 ZStopThread();

    qint32 ZGetCAMImgWidth();
    qint32 ZGetCAMImgHeight();
    qint32 ZGetCAMImgFps();

    QString ZGetDevName();
    bool ZIsRunning();

    QString ZGetCAMID();
signals:
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
    //capture image to local display queue.
    QQueue<QImage> *m_queueDisp;
    QSemaphore *m_semaDispUsed;
    QSemaphore *m_semaDispFree;
    //capture image to process queue.
    QQueue<QImage> *m_queueProcess;
    QSemaphore *m_semaProcessUsed;
    QSemaphore *m_semaProcessFree;
    //capture yuv to yuv queue.
    QQueue<QByteArray> *m_queueYUV;
    QSemaphore *m_semaYUVUsed;
    QSemaphore *m_semaYUVFree;
    bool m_bMainCamera;
private:
    bool m_bExitFlag;
};
#endif // ZIMGCAPTHREAD_H
