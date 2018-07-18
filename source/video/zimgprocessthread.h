#ifndef ZIMGPROCESSTHREAD_H
#define ZIMGPROCESSTHREAD_H

#include <QThread>
#include <QVector>
#include <QMutex>
#include <QtGui/QImage>
#include "zgblpara.h"

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include "zhistogram.h"

#include <QtSerialPort/QSerialPort>
#include <QTimer>

#include <QQueue>
#include <QSemaphore>
#include <QByteArray>


class ZImgProcessThread : public QThread
{
    Q_OBJECT
public:
    ZImgProcessThread();
    ~ZImgProcessThread();
    qint32 ZBindMainQueue(QQueue<QImage> *queue,QSemaphore *semaUsed,QSemaphore *semaFree);
    qint32 ZBindAuxQueue(QQueue<QImage> *queue,QSemaphore *semaUsed,QSemaphore *semaFree);
    qint32 ZBindProcessedSetQueue(QQueue<ZImgProcessedSet> *queue,QSemaphore *semaUsed,QSemaphore *semaFree);
    qint32 ZStartThread();
    qint32 ZStopThread();
    bool ZIsRunning();
protected:
    void run();
signals:
    void ZSigThreadFinished();
    void ZSigMsg(const QString &msg,const qint32 &type);
    void ZSigImgGlobal(QImage img);
    void ZSigImgLocal(QImage img);

    void ZSigTemplateRect(QRect rect);
    void ZSigMatchedRect(QRect rect);

    void ZSigDiffXYT(QRect rectTemp,QRect rectMatched,qint32 nDiffX,qint32 nDiffY,qint32 nCostMs);
    void ZSigSSIMImgSimilarity(qint32 nVal);
    void ZSigObjFeatureKeyPoints(QImage img);
    void ZSigSceneFeatureKeyPoints(QImage img);
public slots:
    void ZSlotSerialPortErr(QSerialPort::SerialPortError err);
    void ZSlotReadUARTData();
public:
    //execute template directly without any processing.
    void ZDoTemplateMatchDirectly(const cv::Mat &mat1,const cv::Mat mat2,const bool bTreatAsGray=false);
    void ZDoTemplateMatchResize(const cv::Mat &mat1,const cv::Mat mat2,const bool bTreatAsGray=false);

    //mat1:the original mat1
    //mat2:the original mat2.
    //mat1Kp:the image1 with key points drawn on it.
    //mat2Kp:the image2 with key points drawn on it.
    qint32 ZDoFeatureDetectMatch(const cv::Mat &matObj,const cv::Mat &matScene,///<
                                 cv::Mat &matObjKp,cv::Mat &matSceneKp,///<
                                 cv::Mat &matResult,cv::Rect rectObj);

private:
    //do PSNR and SSIM to check the image similarity.
    void ZDoPSNRAndSSIM(const cv::Mat &mat1,const cv::Mat &mat2);

    //method1. use histogoram to calculate two images similarity.
    void ZCalcImgHistogram(cv::Mat &img,cv::MatND &bHist,cv::MatND &gHist,cv::MatND &rHist);
    QImage ZGetRGBHistogram(const cv::Mat &mat);

    //method2. use Perceptual hash algorithm.
    qint32 ZCalcPreHash(cv::Mat &mat1,cv::Mat &mat2);

    //Soduko 3x3.
    cv::Mat ZDoSoduKoHistogramCompare(const cv::Mat &matBig,const cv::Mat &matSmall,cv::Rect &rectROI);
    //method4.TemplateMatching.
    //matImg: the original big image.
    //matTemplate: the template small image.
    //nMatchMethod: the opencv3 matchTemplate method.
    cv::Mat ZDoTemplateMatch(const cv::Mat &matImg,const cv::Mat &matTemplate,qint32 nMatchMethod);

    //do gray histogram compare.
    qint32 ZDoHistogramCompare(const cv::Mat &mat1,const cv::Mat &mat2);
private:
    ZHistogram *m_histogram;
private:
    bool m_bExit;
    QMutex m_mux1;
    QMutex m_mux2;
private:
    //mid-value filter.
    QVector<float> m_vecCORREL;

    //mid-value filter for template matching destinate rectangle.
    QVector<cv::Point> m_tempMatchRectVector;

private:
    bool m_bProcessOriginalImg;
    bool m_bProcessGrayImg;
    bool m_bProcessFeaturePoints;
    bool m_bProcessFeatureMatch;

private:
    QSerialPort *m_uart;
    void ZDumpDiffXY2UART(qint32 nDiffX,qint32 nDiffY);
private:
    QTimer *m_timerProcess;
    bool m_bRunning;
private:
    QTimer *m_timerCtl;
    QByteArray m_baUARTRecvBuf;

private:
    QQueue<QImage> *m_queueMain;
    QSemaphore *m_semaMainUsed;
    QSemaphore *m_semaMainFree;

    QQueue<QImage> *m_queueAux;
    QSemaphore *m_semaAuxUsed;
    QSemaphore *m_semaAuxFree;

private:
    QQueue<ZImgProcessedSet> *m_queueProcessedSet;
    QSemaphore *m_semaProcessedSetUsed;
    QSemaphore *m_semaProcessedSetFree;
};

#endif // ZIMGPROCESSTHREAD_H
