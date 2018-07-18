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

class ZVideoTask : public QObject
{
    Q_OBJECT
public:
    explicit ZVideoTask(QObject *parent = nullptr);
    ~ZVideoTask();
    qint32 ZBindMainDispQueue(QQueue<QImage> *queue,QSemaphore *semaUsed,QSemaphore *semaFree);
    qint32 ZBindAuxDispQueue(QQueue<QImage> *queue,QSemaphore *semaUsed,QSemaphore *semaFree);
    qint32 ZBindImgProcessedSet(QQueue<ZImgProcessedSet> *queue,QSemaphore *semaUsed,QSemaphore *semaFree);
    qint32 ZStartTask();
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
    QQueue<QImage> *m_queueMainDisp;
    QSemaphore *m_semaMainDispUsed;
    QSemaphore *m_semaMainDispFree;

    //Aux CaptureThread put QImage to this queue.
    //Aux View Display UI get QImage from this queue for local display.
    QQueue<QImage> *m_queueAuxDisp;
    QSemaphore *m_semaAuxDispUsed;
    QSemaphore *m_semaAuxDispFree;

    //Main CaptureThread put QImage to this queue.
    //ImgProcessThread get QImage from this queue.
    QQueue<QImage> *m_queueMainProcess;
    QSemaphore *m_semaMainProcessUsed;
    QSemaphore *m_semaMainProcessFree;

    //Aux CaptureThread put QImage to this queue.
    //ImgProcessThread get QImage from this queue.
    QQueue<QImage> *m_queueAuxProcess;
    QSemaphore *m_semaAuxProcessUsed;
    QSemaphore *m_semaAuxProcessFree;

    //Main CaptureThread put yuv to this queue.
    //H264 EncodeThread get yuv from this queue.
    QQueue<QByteArray> *m_queueYUV;
    QSemaphore *m_semaYUVUsed;
    QSemaphore *m_semaYUVFree;

    //H264EncThread put h264 frame to this queue.
    //TcpTxThread get data from this queue.
    QQueue<QByteArray> *m_queueH264;
    QSemaphore *m_semaH264Used;
    QSemaphore *m_semaH264Free;

    //ImgProcessThread put data to this queue.
    //Main UI get data from this queue.
    QQueue<ZImgProcessedSet> *m_queueProcessedSet;
    QSemaphore *m_semaProcessedSetUsed;
    QSemaphore *m_semaProcessedSetFree;
};

#endif // ZVIDEOTASK_H
