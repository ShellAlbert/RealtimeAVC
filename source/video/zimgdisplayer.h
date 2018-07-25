#ifndef ZIMGDISPLAYER_H
#define ZIMGDISPLAYER_H

#include <QWidget>
#include <QImage>
#include <QColor>
#include <QToolButton>
#include <QTimer>
#include <QQueue>
#include <QSemaphore>
#include <zgblpara.h>
#include <zringbuffer.h>
#define IMG_SCALED_W    640
#define IMG_SCALED_H    480
class ZImgDisplayer : public QWidget
{
    Q_OBJECT
public:
    explicit ZImgDisplayer(qint32 nCenterX,qint32 nCenterY,bool bMainCamera=false,QWidget *parent = nullptr);
    ~ZImgDisplayer();
    void ZSetSensitiveRect(QRect rect);
    void ZSetPaintParameters(QColor colorRect);
protected:
    QSize sizeHint() const;
    void resizeEvent(QResizeEvent *event);
signals:

public slots:
    void ZSlotFlushImg(const QImage &img);
protected:
    void paintEvent(QPaintEvent *e);

private:
    QImage m_img;
    qint32 m_nCenterX,m_nCenterY;
    QRect m_rectSensitive;
    QRect m_rectSensitiveScaled;
    qint32 m_nSensitiveCenterX,m_nSensitiveCenterY;

    QColor m_colorRect;
    //camera parameters.
    QString m_camID;
    qint32 m_nCamWidth,m_nCamHeight,m_nCAMFps;

    //image counter.
    qint64 m_nFrmCounter;
    //AM i the main camera?
    bool m_bMainCamera;

private:
    //QRect m_rectSensitive;

    qint32 m_nRatio;
    bool m_bStretchFlag;
private:
    //motor move buttons.
    QToolButton *m_tbMotorCtl[4];
private:
    float m_fRatioWidth;
    float m_fRatioHeight;
    QVector<QLineF> m_vecCrossLines;

private:
    //calculate the fps.
    qint64 m_nLastTs;
};

#endif // ZIMGDISPLAYER_H
