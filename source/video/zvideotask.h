#ifndef ZVIDEOTASK_H
#define ZVIDEOTASK_H

#include <QObject>
#include <QPaintEvent>
#include <QImage>
#include <QtWidgets/QWidget>
#include <QLabel>
#include <QQueue>
#include <QByteArray>
#include <QSemaphore>
#include <QProgressBar>

#include <video/zimgcapthread.h>
#include <video/zimgprocessthread.h>
#include <video/zimgdisplayer.h>
#include <video/zh264encthread.h>
#include <video/zvideotxthread.h>

#include <forward/ztcp2uartforwardthread.h>

#include "zgblpara.h"
#include <zringbuffer.h>

class ZVideoTask : public QObject
{
    Q_OBJECT
public:
    explicit ZVideoTask(QObject *parent = nullptr);
    ~ZVideoTask();
    qint32 ZBindMainDispQueue(ZRingBuffer *rbDispMain);
    qint32 ZBindAuxDispQueue(ZRingBuffer *rbDispAux);
    qint32 ZBindImgProcessedSet(QQueue<ZImgProcessedSet> *queue,QSemaphore *semaUsed,QSemaphore *semaFree);
    qint32 ZDoInit();
    qint32 ZStartTask();

    ZImgCapThread* ZGetImgCapThread(qint32 index);
    ZImgProcessThread* ZGetImgProcessThread();
signals:

public slots:
    void ZSlotMsg(const QString &msg,const qint32 &type);
private:
    ZImgCapThread *m_cap1;//主摄像头Main图像采集线程.
    ZImgCapThread *m_cap2;//辅摄像头Aux图像采集线程.
    ZImgProcessThread *m_process;//图像处理线程.
    ZH264EncThread *m_h264Thread;//h264图像编码线程.
    ZVideoTxThread *m_videoTxThread;//图像的TCP传输线程.
    ZTcp2UartForwardThread *m_tcp2Uart;//串口透传线程Android(tcp) <--> STM32(uart).

    //Main CaptureThread put QImage to this queue.
    //Main View Display UI get QImage from this queue for local display.
    ZRingBuffer *m_rbDispMain;

    //Aux CaptureThread put QImage to this queue.
    //Aux View Display UI get QImage from this queue for local display.
    ZRingBuffer *m_rbDispAux;

    //Main CaptureThread put QImage to this queue.
    //ImgProcessThread get QImage from this queue.
    ZRingBuffer *m_rbProcessMain;

    //Aux CaptureThread put QImage to this queue.
    //ImgProcessThread get QImage from this queue.
    ZRingBuffer *m_rbProcessAux;

    //Main CaptureThread put yuv to this queue.
    //H264 EncodeThread get yuv from this queue.
    ZRingBuffer *m_rbYUV;

    //H264EncThread put h264 frame to this queue.
    //TcpTxThread get data from this queue.
    ZRingBuffer *m_rbH264;

    //ImgProcessThread put data to this queue.
    //Main UI get data from this queue.
    QQueue<ZImgProcessedSet> *m_queueProcessedSet;
    QSemaphore *m_semaProcessedSetUsed;
    QSemaphore *m_semaProcessedSetFree;
};

#endif // ZVIDEOTASK_H
