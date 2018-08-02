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
    qint32 ZDoInit();
    qint32 ZStartTask();

    bool ZIsExitCleanup();

    ZImgCapThread* ZGetImgCapThread(qint32 index);
    ZImgProcessThread* ZGetImgProcessThread();
signals:
    void ZSigVideoTaskExited();
private slots:
    void ZSlotSubThreadsExited();
    void ZSlotChkAllExitFlags();
private:
    ZImgCapThread *m_capThread[2];//主/辅摄像头Main/Aux图像采集线程.
    ZImgProcessThread *m_process;//图像处理线程.
    ZVideoTxThread *m_videoTxThread[2];//h264图像编码&TCP传输线程.
    ZTcp2UartForwardThread *m_tcp2Uart;//串口透传线程Android(tcp) <--> STM32(uart).

    //Main/Aux CaptureThread put QImage to this queue.
    //ImgProcessThread get QImage from this queue.
    ZRingBuffer *m_rbProcess[2];

    //Main CaptureThread put yuv to this queue.
    //H264 EncodeThread get yuv from this queue.
    ZRingBuffer *m_rbYUV[2];

    //H264EncThread put h264 frame to this queue.
    //TcpTxThread get data from this queue.
    ZRingBuffer *m_rbH264[2];
private:
    QTimer *m_timerExit;
};

#endif // ZVIDEOTASK_H
