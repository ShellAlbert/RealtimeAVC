#include "zimgdisplayer.h"
#include <QPainter>
#include <QDebug>
ZImgDisplayer::ZImgDisplayer(qint32 nCenterX,qint32 nCenterY,bool bMainCamera,QWidget *parent) : QWidget(parent)
{
    this->m_nCenterX=nCenterX;
    this->m_nCenterY=nCenterY;
    this->m_nTriggerCounter=0;
    this->m_colorRect=QColor(0,255,0);
    this->m_bMainCamera=bMainCamera;
    this->m_nSensitiveCenterX=0;
    this->m_nSensitiveCenterY=0;
    //we supply the motor move buttons for SlaveCamera.
    if(!this->m_bMainCamera)
    {
        for(qint32 i=0;i<4;i++)
        {
            this->m_tbMotorCtl[i]=new QToolButton(this);
            this->m_tbMotorCtl[i]->setIconSize(QSize(48,48));
            this->m_tbMotorCtl[i]->setStyleSheet("QToolButton{background:transparent;border-style:flat;}");
            switch(i)
            {
            case 0:
                this->m_tbMotorCtl[i]->setIcon(QIcon(":/motor/images/motor/up.png"));
                break;
            case 1:
                this->m_tbMotorCtl[i]->setIcon(QIcon(":/motor/images/motor/down.png"));
                break;
            case 2:
                this->m_tbMotorCtl[i]->setIcon(QIcon(":/motor/images/motor/left.png"));
                break;
            case 3:
                this->m_tbMotorCtl[i]->setIcon(QIcon(":/motor/images/motor/right.png"));
                break;
            }
        }
    }

    this->m_queue=NULL;
    this->m_semaUsed=NULL;
    this->m_semaFree=NULL;
}
void ZImgDisplayer::ZSetSensitiveRect(QRect rect)
{
    this->m_rectSensitive=rect;
    //scaled rectangle to adapt current window size.
    this->m_rectSensitiveScaled.setX(this->m_rectSensitive.x()*this->m_fRatioWidth);
    this->m_rectSensitiveScaled.setY(this->m_rectSensitive.y()*this->m_fRatioHeight);
    this->m_rectSensitiveScaled.setWidth(this->m_rectSensitive.width()*this->m_fRatioWidth);
    this->m_rectSensitiveScaled.setHeight(this->m_rectSensitive.height()*this->m_fRatioHeight);
    //得到区域的中心点坐标.
    this->m_nSensitiveCenterX=this->m_rectSensitiveScaled.x()+this->m_rectSensitiveScaled.width()/2;
    this->m_nSensitiveCenterY=this->m_rectSensitiveScaled.y()+this->m_rectSensitiveScaled.height()/2;
}
void ZImgDisplayer::ZSetCAMParameters(qint32 nWidth,qint32 nHeight,qint32 nFps,QString camID)
{
    this->m_nCAMFps=15;
    this->m_camID=camID;
}
void ZImgDisplayer::ZSetPaintParameters(QColor colorRect)
{
    this->m_colorRect=colorRect;
}
void ZImgDisplayer::ZBindQueue(QQueue<QImage> *queue,QSemaphore *semaUsed,QSemaphore *semaFree)
{
    this->m_queue=queue;
    this->m_semaUsed=semaUsed;
    this->m_semaFree=semaFree;

    //start timer.
    this->m_timer=new QTimer;
    QObject::connect(this->m_timer,SIGNAL(timeout()),this,SLOT(ZSlotFetchImg()));
    this->m_timer->start(10);//30ms.
    return;
}
QSize ZImgDisplayer::sizeHint() const
{
    return QSize(IMG_SCALED_W,IMG_SCALED_H);
}
void ZImgDisplayer::resizeEvent(QResizeEvent *event)
{
    if(!this->m_bMainCamera)
    {
        //calculate the center point.
        //    X
        //  X  X
        //   X
        QPoint ptCenter;
        ptCenter.setX(this->width()-48*3);
        ptCenter.setY((this->height()-48*3)/2);
        //because our icons are 48x48.
        this->m_tbMotorCtl[0]->move(ptCenter.x(),ptCenter.y()-48);//up.
        this->m_tbMotorCtl[1]->move(ptCenter.x(),ptCenter.y()+48);//down.
        this->m_tbMotorCtl[2]->move(ptCenter.x()-48,ptCenter.y());//left.
        this->m_tbMotorCtl[3]->move(ptCenter.x()+48,ptCenter.y());//right.
    }

    //摄像头捕获的图像分辨率，这里固定写死。
    this->m_nCamWidth=640;
    this->m_nCamHeight=480;

    //    qDebug("window size:%d*%d\n",this->width(),this->height());
    //    qDebug("cam size:%d*%d\n",this->m_nCamWidth,this->m_nCamHeight);

    //the cross + size,width*height.
    qint32 nCrossW=20;
    qint32 nCrossH=20;
    //the skip pixels,empty black.
    qint32 nSkipPixels=6;

    //图像因绽放会变形，此处计算出图像变形的x比例,y比例.
    //根据比较调整要绘制的中心点框线的坐标.
    //calculate the scale ratio for x.
    if(this->width()>this->m_nCamWidth)
    {
        this->m_fRatioWidth=this->width()/(this->m_nCamWidth*1.0);
    }else if(this->width()<this->m_nCamWidth)
    {
        this->m_fRatioWidth=this->m_nCamWidth/(this->width()*1.0);
    }else{
        this->m_fRatioWidth=1.0f;
    }
    //calculate the scale ratio for y.
    if(this->height()>this->m_nCamHeight)
    {
        this->m_fRatioHeight=this->height()/(this->m_nCamHeight*1.0);
    }else if(this->height()<this->m_nCamHeight){
        this->m_fRatioHeight=this->m_nCamHeight/(this->height()*1.0);
    }else{
        this->m_fRatioHeight=1.0f;
    }

    //clear vector lines.
    this->m_vecCrossLines.clear();

    //calculate the top part,from top to bottom.
    //计算左边线的两个端点坐标.
    QPointF ptTop,ptTop2;
    if(this->m_nCenterY*this->m_fRatioHeight-nCrossH<0)
    {
        //reaches the boundary,no need to draw.
    }else{
        ptTop=QPointF(this->m_nCenterX*this->m_fRatioWidth,this->m_nCenterY*this->m_fRatioHeight-nCrossH);
        ptTop2=QPointF(this->m_nCenterX*this->m_fRatioWidth,this->m_nCenterY*this->m_fRatioHeight-nSkipPixels);
        this->m_vecCrossLines.append(QLineF(ptTop,ptTop2));
    }

    //calculate the bottom part,from bottom to top.
    //计算底边线的两个端点的坐标.
    QPointF ptBottom,ptBottom2;
    if(this->m_nCenterY*this->m_fRatioHeight+nCrossH>this->height())
    {
        //reaches the boundary,no need to draw.
    }else{
        ptBottom=QPointF(this->m_nCenterX*this->m_fRatioWidth,this->m_nCenterY*this->m_fRatioHeight+nCrossH);
        ptBottom2=QPointF(this->m_nCenterX*this->m_fRatioWidth,this->m_nCenterY*this->m_fRatioHeight+nSkipPixels);
        this->m_vecCrossLines.append(QLineF(ptBottom,ptBottom2));
    }

    //draw the left part,from left to right.
    //计算左边线的两个端点的坐标.
    QPointF ptLeft,ptLeft2;
    if(this->m_nCenterX*this->m_fRatioWidth-nCrossW<0)
    {
        //reaches the boundary,no need to draw.
    }else{
        ptLeft=QPointF(this->m_nCenterX*this->m_fRatioWidth-nCrossW,this->m_nCenterY*this->m_fRatioHeight);
        ptLeft2=QPointF(this->m_nCenterX*this->m_fRatioWidth-nSkipPixels,this->m_nCenterY*this->m_fRatioHeight);
        this->m_vecCrossLines.append(QLineF(ptLeft,ptLeft2));
    }

    //draw the right part.
    //计算右边线的两个端点的坐标.
    QPointF ptRight,ptRight2;
    if(this->m_nCenterX*this->m_fRatioWidth+nCrossW>this->width())
    {
        //reaches the boundary,no need to draw.
    }else{
        ptRight=QPointF(this->m_nCenterX*this->m_fRatioWidth+nCrossW,this->m_nCenterY*this->m_fRatioHeight);
        ptRight2=QPointF(this->m_nCenterX*this->m_fRatioWidth+nSkipPixels,this->m_nCenterY*this->m_fRatioHeight);
        this->m_vecCrossLines.append(QLineF(ptRight,ptRight2));
    }

    QWidget::resizeEvent(event);
}
void ZImgDisplayer::ZSlotFetchImg()
{
    //paint now.
    this->m_nTriggerCounter++;
    this->update();
}
void ZImgDisplayer::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);

    QImage newImg;
    this->m_semaUsed->acquire();//已用信号量减1.
    newImg=this->m_queue->dequeue();
    this->m_semaFree->release();//空闲信号量加1.

    QPainter painter(this);
    if(newImg.isNull())
    {
        painter.fillRect(QRectF(0,0,this->width(),this->height()),Qt::black);
        QPen pen(Qt::green,4);
        painter.setPen(pen);
        painter.drawLine(QPointF(0,0),QPointF(this->width(),this->height()));
        painter.drawLine(QPointF(this->width(),0),QPointF(0,this->height()));
        return;
    }
    //draw the image.
    QRectF rectIMG(0,0,this->width(),this->height());
    painter.drawImage(rectIMG,newImg);

    //set font & pen.
    QFont tFont=painter.font();
    tFont.setPointSize(16);
    painter.setFont(tFont);
    painter.setPen(QPen(Qt::green,3,Qt::SolidLine));

    //将需要绘制的线条集中打包Vector一次性绘制节省时间.
    //draw the center cross + indicator.
    painter.drawLines(this->m_vecCrossLines);

    //draw the camera sensitive rectangle.
    QPen penRect(Qt::red,4,Qt::DashLine);
    painter.setPen(penRect);
    painter.drawRect(this->m_rectSensitiveScaled);

    //对于辅助镜头来讲，绘制一条直接从校正坐标原点到匹配到的区域中心点.
    painter.drawLine(QPointF(this->m_nSensitiveCenterX,this->m_nSensitiveCenterY),QPointF(this->m_nCenterX*this->m_fRatioWidth,this->m_nCenterY*this->m_fRatioHeight));

    //draw the camera resolution & fps.
    //    QString camInfo;
    //    camInfo.append(tr("%1*%2\n").arg(this->m_nCamWidth).arg(this->m_nCamHeight));
    //    camInfo.append(tr("%1fps\n").arg(this->m_nCAMFps));
    //    camInfo.append(QString::number(this->m_nTriggerCounter,10));
    //    //here plus 10 to avoid last text missing.
    //    QRect rectCAMInfo(0,0,painter.fontMetrics().width(camInfo)+10,painter.fontMetrics().height()*3);//3 lines.
    //    painter.drawText(rectCAMInfo,camInfo);



#if 0



    //////////////////////////////////////////////////////////////////////////////////
    //draw the camID.
    qint32 nWidthCAMID=painter.fontMetrics().width(this->m_camID);
    qint32 nHeightCAMID=painter.fontMetrics().height();
    //here plus 10 to avoid last text missing.
    QRect rectCAMID(this->width()-nWidthCAMID,this->height()-nHeightCAMID,nWidthCAMID+10,nHeightCAMID);
    painter.drawText(rectCAMID,this->m_camID);
#endif
#if 0
    //////////////////////////////////////////////////////////////////////////////////
    //if i am the main camera.
    if(this->m_bMainCamera)
    {
        qint32 nFontHeight=painter.fontMetrics().height();

        QString mainTips("Main");
        qint32 nMainTipsWidth=painter.fontMetrics().width(mainTips);
        //here plus 10 to avoid the last text missing.
        QRect rectMainTips(this->width()-nMainTipsWidth,0,nMainTipsWidth+10,nFontHeight);
        painter.drawText(rectMainTips,mainTips);

        //draw the calibration (x,y) text.
        QString calibrateXY;
        calibrateXY.append("Calibrated:(");
        calibrateXY.append(QString::number(this->m_nCenterX,10));
        calibrateXY.append(",");
        calibrateXY.append(QString::number(this->m_nCenterY,10));
        calibrateXY.append(")");

        qint32 nCalibrateWidth=painter.fontMetrics().width(calibrateXY);
        //QString str("Calibrate:(320,240)");
        //qDebug()<<calibrateXY<<"="<<nCalibrateWidth<<","<<painter.fontMetrics().width(str);
        //here plus 10 to avoid the last text missing.
        QRect rectCalibrateXY(0,this->height()-nFontHeight,nCalibrateWidth+10,nFontHeight);
        painter.drawText(rectCalibrateXY,calibrateXY);

        //draw the sensitive rectangle.
        QPen penRect(this->m_colorRect,4,Qt::DashLine);
        painter.setPen(penRect);
        QRect rectSensitiveRatio;
        rectSensitiveRatio.setX(this->m_rectSensitiveOld.x()*fRatioWidth);
        rectSensitiveRatio.setY(this->m_rectSensitiveOld.y()*fRatioHeight);
        rectSensitiveRatio.setWidth(this->m_rectSensitiveOld.width()*fRatioWidth);
        rectSensitiveRatio.setHeight(this->m_rectSensitiveOld.height()*fRatioHeight);
        painter.drawRect(rectSensitiveRatio);

    }else{

        //i am the aux camera.
        qint32 nFontHeight=painter.fontMetrics().height();

        QString auxTips("Aux");
        qint32 nAuxTipsWidth=painter.fontMetrics().width(auxTips);
        //here plus 10 to avoid the last text missing.
        QRect rectAuxTips(this->width()-nAuxTipsWidth,0,nAuxTipsWidth+10,nFontHeight);
        painter.drawText(rectAuxTips,auxTips);

        //draw the calibration (x,y) text.
        QString calibrateXY("Calibrated:(");
        calibrateXY.append(QString::number(this->m_nCenterX,10));
        calibrateXY.append(",");
        calibrateXY.append(QString::number(this->m_nCenterY,10));
        calibrateXY.append(")->(");
        calibrateXY.append(QString::number(this->m_rectSensitive.x(),10));
        calibrateXY.append(",");
        calibrateXY.append(QString::number(this->m_rectSensitive.y(),10));
        calibrateXY.append(")");
        qint32 nCalibrateWidth=painter.fontMetrics().width(calibrateXY);
        //here plus 10 to avoid the last text missing.
        QRect rectCalibrateXY(0,this->height()-nFontHeight,nCalibrateWidth+10,nFontHeight);
        painter.drawText(rectCalibrateXY,calibrateXY);

        //stretch rectangle or not?
        if(this->m_bStretchFlag)
        {
            qint32 newX=this->m_rectSensitiveOld.x()+this->m_nRatio*10;
            qint32 newY=this->m_rectSensitiveOld.y()+this->m_nRatio*10;
            qint32 newWidth=this->m_rectSensitiveOld.width()-this->m_nRatio*10*2;
            qint32 newHeight=this->m_rectSensitiveOld.height()-this->m_nRatio*10*2;
            if(newWidth>10 && newHeight>10)
            {
                this->m_rectSensitiveOld=QRect(newX,newY,newWidth,newHeight);
                this->m_nRatio++;
            }else{
                this->m_rectSensitiveOld=this->m_rectSensitive;
                this->m_nRatio=1;
            }
        }

        //draw the sensitive rectangle.
        QPen penRect(this->m_colorRect,4,Qt::DashLine);
        painter.setPen(penRect);
        QRect rectSensitiveRatio;
        rectSensitiveRatio.setX(this->m_rectSensitiveOld.x()*fRatioWidth);
        rectSensitiveRatio.setY(this->m_rectSensitiveOld.y()*fRatioHeight);
        rectSensitiveRatio.setWidth(this->m_rectSensitiveOld.width()*fRatioWidth);
        rectSensitiveRatio.setHeight(this->m_rectSensitiveOld.height()*fRatioHeight);
        painter.drawRect(rectSensitiveRatio);

        //draw the center of sensitive rectangle.
        qint32 nSenseCenterX=rectSensitiveRatio.x()+rectSensitiveRatio.width()/2;
        qint32 nSenseCenterY=rectSensitiveRatio.y()+rectSensitiveRatio.height()/2;
        //draw the top part.
        QPoint ptSenseTop;
        if(nSenseCenterX-nCrossH<0)
        {
            ptSenseTop=QPoint(nSenseCenterX,nSenseCenterY);
        }else{
            ptSenseTop=QPoint(nSenseCenterX,nSenseCenterY-nCrossH);
            QPoint ptSenseTop2(nSenseCenterX,nSenseCenterY-nSkipPixels);
            painter.drawLine(ptSenseTop,ptSenseTop2);
        }
        //draw the bottom part.
        QPoint ptSenseBottom;
        if(nSenseCenterY+nCrossH>this->height())
        {
            ptSenseBottom=QPoint(nSenseCenterX,nSenseCenterY);
        }else{
            ptSenseBottom=QPoint(nSenseCenterX,nSenseCenterY+nCrossH);
            QPoint ptSenseBottom2(nSenseCenterX,nSenseCenterY+nSkipPixels);
            painter.drawLine(ptSenseBottom,ptSenseBottom2);
        }
        //draw the left part.
        QPoint ptSenseLeft;
        if(nSenseCenterX-nCrossW<0)
        {
            ptSenseLeft=QPoint(nSenseCenterX,nSenseCenterY);
        }else{
            ptSenseLeft=QPoint(nSenseCenterX-nCrossW,nSenseCenterY);
            QPoint ptSenseLeft2(nSenseCenterX-nSkipPixels,nSenseCenterY);
            painter.drawLine(ptSenseLeft,ptSenseLeft2);
        }
        //draw the right part.
        QPoint ptSenseRight;
        if(nSenseCenterX+nCrossW>this->width())
        {
            ptSenseRight=QPoint(nSenseCenterX,nSenseCenterY);
        }else{
            ptSenseRight=QPoint(nSenseCenterX+nCrossW,nSenseCenterY);
            QPoint ptSenseRight2(nSenseCenterX+nSkipPixels,nSenseCenterY);
            painter.drawLine(ptSenseRight,ptSenseRight2);
        }

        //draw a line from Calibrate center(x,y) to sensitive rectangle center(x,y).
        QPoint ptCalibrateXY(this->m_nCenterX*fRatioWidth,this->m_nCenterY*fRatioHeight);
        QPoint ptSensRectXY(rectSensitiveRatio.x()+rectSensitiveRatio.width()/2,rectSensitiveRatio.y()+rectSensitiveRatio.height()/2);
        if(ptCalibrateXY.x()>=0 && ptCalibrateXY.x()<=this->width() && ptCalibrateXY.y()>=0 && ptCalibrateXY.y()<=this->height())
        {
            if(ptSensRectXY.x()>=0 && ptSensRectXY.x()<=this->width() && ptSensRectXY.y()>=0 && ptSensRectXY.y()<=this->height())
            {
                QPen penLine(Qt::yellow,3,Qt::SolidLine);
                painter.setPen(penLine);
                painter.drawLine(ptCalibrateXY,ptSensRectXY);
            }
        }

    }
#endif
    //////////////////////////////////////////////////////////////////////////////////
}
