#ifndef ZAVUI_H
#define ZAVUI_H
#include <QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QPaintEvent>
#include <QImage>
#include <QtWidgets/QWidget>
#include <QLabel>
#include <QQueue>
#include <QByteArray>
#include <QSemaphore>
#include <QProgressBar>
#include <video/zimgdisplayer.h>
#include "zgblpara.h"
#include <QtCharts/QLineSeries>
#include <QtCharts/QtCharts>
#include "xyseriesiodevice.h"
#include <zringbuffer.h>
class ZAVUI : public QFrame
{
    Q_OBJECT
public:
    ZAVUI(QWidget *parent=nullptr);
    ~ZAVUI();
    qint32 ZBindMainDispQueue(ZRingBuffer *rbDispMain);
    qint32 ZBindAuxDispQueue(ZRingBuffer *rbDispAux);
    qint32 ZBindImgProcessedSet(QQueue<ZImgProcessedSet> *queue,QSemaphore *semaUsed,QSemaphore *semaFree);

    qint32 ZBindWaveFormQueueBefore(ZRingBuffer *rbWaveBefore);
    qint32 ZBindWaveFormQueueAfter(ZRingBuffer *rbWaveAfter);

    ZImgDisplayer *ZGetImgDisp(qint32 index);
private slots:
    void ZSlotSSIMImgSimilarity(qint32 nVal);
    void ZSlot1sTimeout();
public slots:
    void ZSlotFlushWaveBefore();
    void ZSlotFlushWaveAfter();
    void ZSlotFlushProcessedSet();
private:
    void ZUpdateMatchBar(QProgressBar *pBar,qint32 nVal);
protected:
    void closeEvent(QCloseEvent *event);
private:
    //ImgProcessThread put data to this queue.
    //Main UI get data from this queue.
    QQueue<ZImgProcessedSet> *m_queueProcessedSet;
    QSemaphore *m_semaProcessedSetUsed;
    QSemaphore *m_semaProcessedSetFree;
private:
    //像素坐标差值及算法消耗时间.
    QLabel *m_llDiffXY;
    //程序累计运行时间.
    QLabel *m_llRunSec;
    QTimer *m_timer;
    qint64 m_nTimeCnt;
    //算法当前状态，开启/关闭.
    QLabel *m_llState;
    //透传数据统计Android(tcp) <--> STM32(uart).
    QLabel *m_llForward;
    //音频客户端是否连接/视频客户端是否连接.
    QLabel *m_llAVClients;
    //layout.
    QVBoxLayout *m_vLayoutInfo;


    //the bottom layout.
    //disp0,progress bar,disp1.
    ZImgDisplayer *m_disp[2];
    QProgressBar *m_SSIMMatchBar;

    QHBoxLayout *m_hLayout;
    QVBoxLayout *m_vLayout;

    QVBoxLayout *m_vLayoutWavForm;
    QHBoxLayout *m_hLayoutWavFormAndInfo;
private:
    QLineSeries *m_seriesBefore;
    XYSeriesIODevice *m_deviceBefore;
    QChartView *m_chartViewBefore;
    QChart *m_chartBefore;

    QLineSeries *m_seriesAfter;
    XYSeriesIODevice *m_deviceAfter;
    QChartView *m_chartViewAfter;
    QChart *m_chartAfter;
private:
    //波形显示队列，降噪算法处理之前与处理之后波形比对.
    ZRingBuffer *m_rbWaveBefore;
    ZRingBuffer *m_rbWaveAfter;
};

#endif // ZAVUI_H
