#include "zmaintask.h"
#include "zfiltecamdev.h"
#include <QStringList>
#include <QDebug>
#include <QDateTime>
#include <QPainter>
#include <QApplication>
ZMainTask::ZMainTask(QWidget *parent) : QFrame(parent)
{

    this->m_cap1=NULL;
    this->m_cap2=NULL;
    this->m_process=NULL;
    this->m_videoTxThread=NULL;
    this->m_tcp2Uart=NULL;

    this->m_llDiffXY=new QLabel;
    this->m_llDiffXY->setAlignment(Qt::AlignCenter);
    this->m_llDiffXY->setText(tr("Diff XYT\n[X:0 Y:0 T:0ms]"));
    this->m_llRunSec=new QLabel;
    this->m_llRunSec->setAlignment(Qt::AlignCenter);
    this->m_llRunSec->setText(tr("Accumulated\n[0H0M0S]"));
    this->m_llState=new QLabel;
    this->m_llState->setAlignment(Qt::AlignCenter);
    this->m_llState->setText(tr("State\n[Stop]"));
    this->m_hLayoutInfo=new QHBoxLayout;
    this->m_hLayoutInfo->addWidget(this->m_llDiffXY);
    this->m_hLayoutInfo->addWidget(this->m_llRunSec);
    this->m_hLayoutInfo->addWidget(this->m_llState);

    this->m_llDiffXY->setStyleSheet("QLabel{background:#FFAF60;font:50px;min-width:200px;}");
    this->m_llRunSec->setStyleSheet("QLabel{background:#9999CC;font:50px;min-width:200px;}");
    this->m_llState->setStyleSheet("QLabel{background:#AFAF61;font:50px;min-width:200px;}");

    this->m_SSIMMatchBar=new QProgressBar;
    this->m_SSIMMatchBar->setOrientation(Qt::Horizontal);
    this->m_SSIMMatchBar->setValue(0);
    this->m_SSIMMatchBar->setRange(0,100);
    this->m_SSIMMatchBar->setStyleSheet("QProgressBar::chunk{background-color:#FF0000}");
    this->m_SSIMMatchBar->setFormat(tr("SSIM %p%"));

    this->m_disp[0]=new ZImgDisplayer(gGblPara.m_calibrateX1,gGblPara.m_calibrateY1,true);//the main camera.
    this->m_disp[1]=new ZImgDisplayer(gGblPara.m_calibrateX2,gGblPara.m_calibrateY2);
    this->m_disp[0]->ZSetPaintParameters(QColor(0,255,0));
    this->m_disp[1]->ZSetPaintParameters(QColor(255,0,0));
    this->m_hLayout=new QHBoxLayout;
    this->m_hLayout->addWidget(this->m_disp[0]);
    this->m_hLayout->addWidget(this->m_disp[1]);
    this->m_vLayout=new QVBoxLayout;
    this->m_vLayout->addLayout(this->m_hLayoutInfo);
    this->m_vLayout->addWidget(this->m_SSIMMatchBar);
    this->m_vLayout->addLayout(this->m_hLayout);
    this->setLayout(this->m_vLayout);

    this->setStyleSheet("QFrame{background-color:#84C1FF;}");

    //running time.
    this->m_nTimeCnt=0;
    this->m_timer=new QTimer;
    QObject::connect(this->m_timer,SIGNAL(timeout()),this,SLOT(ZSlot1sTimeout()));
    this->m_timer->start(1000);

    //fetch processed set timer.
    this->m_timerDispatch=new QTimer;
    QObject::connect(this->m_timerDispatch,SIGNAL(timeout()),this,SLOT(ZSlotDispatchProcessedSet()));
    this->m_timerDispatch->start(20);
}
ZMainTask::~ZMainTask()
{
    if(this->m_process)
    {
        this->m_process->ZStopThread();
        this->m_process->wait(1000*10);//10s.
        delete this->m_process;
    }

    if(this->m_cap1)
    {
        this->m_cap1->ZStopThread();
        this->m_cap1->wait(1000*10);//10s.
        delete this->m_cap1;
    }

    if(this->m_cap2)
    {
        this->m_cap2->ZStopThread();
        this->m_cap2->wait(1000*10);//10s.
        delete this->m_cap2;
    }

    delete this->m_llDiffXY;
    delete this->m_llRunSec;
    delete this->m_llState;
    delete this->m_hLayoutInfo;

    delete this->m_SSIMMatchBar;
    delete this->m_disp[0];
    delete this->m_disp[1];
    delete this->m_hLayout;
    delete this->m_vLayout;

    this->m_timer->stop();
    delete this->m_timer;
}
qint32 ZMainTask::ZStartTask()
{
    ZFilteCAMDev filteCAMDev;
    QStringList lstRealDev=filteCAMDev.ZGetCAMDevList();
    if(lstRealDev.size()<2)
    {
        if(gGblPara.m_bVerbose)
        {
            qDebug()<<"<error>:cannot find two Video Nodes!";
        }
        return -1;
    }

    //start cap thread.
    QString cam1Device("/dev/"+lstRealDev.at(0));
    QString cam2Device("/dev/"+lstRealDev.at(1));
    if(gGblPara.m_bVerbose)
    {
        QString msgVerbose=QString("camera 1=%1,camera 2=%2").arg(cam1Device).arg(cam2Device);
        this->ZSlotMsg(msgVerbose,Log_Msg_Info);
    }

    /////////////////////////////the main camera queue///////////////////////////
    //main capture thread to local display queue.
    this->m_queueMainDisp=new QQueue<QImage>;
    this->m_semaMainDispUsed=new QSemaphore(0);
    this->m_semaMainDispFree=new QSemaphore(15);//30fps,5sec.
    //main capture thread to img process queue.
    this->m_queueMainProcess=new QQueue<QImage>;
    this->m_semaMainProcessUsed=new QSemaphore(0);
    this->m_semaMainProcessFree=new QSemaphore(15);//30fps,5sec.
    //main capture thread to yuv queue.
    this->m_queueYUV=new QQueue<QByteArray>;
    this->m_semaYUVUsed=new QSemaphore(0);
    this->m_semaYUVFree=new QSemaphore(15);

    ////////////////////////////////the aux camera queue///////////////////////////
    //aux capture thread to local display queue.
    this->m_queueAuxDisp=new QQueue<QImage>;
    this->m_semaAuxDispUsed=new QSemaphore(0);
    this->m_semaAuxDispFree=new QSemaphore(15);
    //aux capture thread to img process queue.
    this->m_queueAuxProcess=new QQueue<QImage>;
    this->m_semaAuxProcessUsed=new QSemaphore(0);
    this->m_semaAuxProcessFree=new QSemaphore(15);

    ///////////////////////////////*h264 encode result queue/////////////////////////////
    this->m_queueH264=new QQueue<QByteArray>;
    this->m_semaH264Used=new QSemaphore(0);
    this->m_semaH264Free=new QSemaphore(15);

    /////////////////////////img process thread result queue/////////////////////////
    this->m_queueProcessedSet=new QQueue<ZImgProcessedSet>;
    this->m_semaProcessedSetUsed=new QSemaphore(0);
    this->m_semaProcessedSetFree=new QSemaphore(15);

    //bind queue to local display.
    this->m_disp[0]->ZBindQueue(this->m_queueMainDisp,this->m_semaMainDispUsed,this->m_semaMainDispFree);
    this->m_disp[1]->ZBindQueue(this->m_queueAuxDisp,this->m_semaAuxDispUsed,this->m_semaAuxDispFree);

    //create capture thread.
    this->m_cap1=new ZImgCapThread(cam1Device,gGblPara.m_widthCAM1,gGblPara.m_heightCAM1,gGblPara.m_fpsCAM1,true);//Main Camera.
    this->m_cap1->ZBindDispQueue(this->m_queueMainDisp,this->m_semaMainDispUsed,this->m_semaMainDispFree);
    this->m_cap1->ZBindProcessQueue(this->m_queueMainProcess,this->m_semaMainProcessUsed,this->m_semaMainProcessFree);
    this->m_cap1->ZBindYUVQueue(this->m_queueYUV,this->m_semaYUVUsed,this->m_semaYUVFree);

    this->m_cap2=new ZImgCapThread(cam2Device,gGblPara.m_widthCAM2,gGblPara.m_heightCAM2,gGblPara.m_fpsCAM2,false);
    this->m_cap2->ZBindDispQueue(this->m_queueAuxDisp,this->m_semaAuxDispUsed,this->m_semaAuxDispFree);
    this->m_cap2->ZBindProcessQueue(this->m_queueAuxProcess,this->m_semaAuxProcessUsed,this->m_semaAuxProcessFree);

    //create h264 encode thread.
    this->m_h264Thread=new ZH264EncThread;
    this->m_h264Thread->ZBindYUVQueue(this->m_queueYUV,this->m_semaYUVUsed,this->m_semaYUVFree);
    this->m_h264Thread->ZBindH264Queue(this->m_queueH264,this->m_semaH264Used,this->m_semaH264Free);

    //create image process thread.
    this->m_process=new ZImgProcessThread;
    this->m_process->ZBindMainQueue(this->m_queueMainProcess,this->m_semaMainProcessUsed,this->m_semaMainProcessFree);
    this->m_process->ZBindAuxQueue(this->m_queueAuxProcess,this->m_semaAuxProcessUsed,this->m_semaAuxProcessFree);
    this->m_process->ZBindProcessedSetQueue(this->m_queueProcessedSet,this->m_semaProcessedSetUsed,this->m_semaProcessedSetFree);

    //start tcp server.
    this->m_videoTxThread=new ZVideoTxThread;
    this->m_videoTxThread->ZBindQueue(this->m_queueH264,this->m_semaH264Used,this->m_semaH264Free);
    this->m_videoTxThread->ZStartThread();

    //start h264 thread.
    this->m_h264Thread->ZStartThread();

    //start img process thread.
    this->m_process->ZStartThread();

    //start capture thread.
    this->m_cap1->ZStartThread();
    this->m_cap2->ZStartThread();

    return 0;
#if 0
    this->m_process=new ZImgProcessThread;
    QString cap1ID=this->m_cap1->ZGetCAMID();
    QString cap2ID=this->m_cap2->ZGetCAMID();
    if(cap1ID==gGblPara.m_idCAM1 && cap2ID==gGblPara.m_idCAM2)
    {
        this->m_disp[0]->ZSetCAMParameters(gGblPara.m_widthCAM1,gGblPara.m_heightCAM1,gGblPara.m_fpsCAM1,cap1ID);//main camera.
        this->m_disp[1]->ZSetCAMParameters(gGblPara.m_widthCAM2,gGblPara.m_heightCAM2,gGblPara.m_fpsCAM2,cap2ID);

        if(gGblPara.m_bFMode)
        {
            //F mode.
            QObject::connect(this->m_process,SIGNAL(ZSigObjFeatureKeyPoints(QImage)),this->m_disp[0],SLOT(ZSlotDispImg(QImage)));
            QObject::connect(this->m_process,SIGNAL(ZSigSceneFeatureKeyPoints(QImage)),this->m_disp[1],SLOT(ZSlotDispImg(QImage)));
        }else{
            //X mode.
            QObject::connect(this->m_cap1,SIGNAL(ZSigCapImg(QImage)),this->m_disp[0],SLOT(ZSlotDispImg(QImage)));
            QObject::connect(this->m_cap2,SIGNAL(ZSigCapImg(QImage)),this->m_disp[1],SLOT(ZSlotDispImg(QImage)));
        }

        QObject::connect(this->m_cap1,SIGNAL(ZSigCapImg(QImage)),this->m_process,SLOT(ZSlotGetImg1(QImage)));
        QObject::connect(this->m_cap2,SIGNAL(ZSigCapImg(QImage)),this->m_process,SLOT(ZSlotGetImg2(QImage)));
    }else if(cap1ID==gGblPara.m_idCAM2 && cap2ID==gGblPara.m_idCAM1){

        this->m_disp[0]->ZSetCAMParameters(gGblPara.m_widthCAM2,gGblPara.m_heightCAM2,gGblPara.m_fpsCAM2,cap2ID);//main camera.
        this->m_disp[1]->ZSetCAMParameters(gGblPara.m_widthCAM1,gGblPara.m_heightCAM1,gGblPara.m_fpsCAM1,cap1ID);

        if(gGblPara.m_bFMode)
        {
            //F mode.
            QObject::connect(this->m_process,SIGNAL(ZSigSceneFeatureKeyPoints(QImage)),this->m_disp[0],SLOT(ZSlotDispImg(QImage)));
            QObject::connect(this->m_process,SIGNAL(ZSigObjFeatureKeyPoints(QImage)),this->m_disp[1],SLOT(ZSlotDispImg(QImage)));
        }else{
            //X mode.
            QObject::connect(this->m_cap2,SIGNAL(ZSigCapImg(QImage)),this->m_disp[0],SLOT(ZSlotDispImg(QImage)));
            QObject::connect(this->m_cap1,SIGNAL(ZSigCapImg(QImage)),this->m_disp[1],SLOT(ZSlotDispImg(QImage)));
        }

        QObject::connect(this->m_cap2,SIGNAL(ZSigCapImg(QImage)),this->m_process,SLOT(ZSlotGetImg1(QImage)));
        QObject::connect(this->m_cap1,SIGNAL(ZSigCapImg(QImage)),this->m_process,SLOT(ZSlotGetImg2(QImage)));
    }else{ //anyway here.

        this->m_disp[0]->ZSetCAMParameters(gGblPara.m_widthCAM1,gGblPara.m_heightCAM1,gGblPara.m_fpsCAM1,cap1ID);//main camera.
        this->m_disp[1]->ZSetCAMParameters(gGblPara.m_widthCAM2,gGblPara.m_heightCAM2,gGblPara.m_fpsCAM2,cap2ID);

        if(gGblPara.m_bFMode)
        {
            //F mode.
            QObject::connect(this->m_process,SIGNAL(ZSigObjFeatureKeyPoints(QImage)),this->m_disp[0],SLOT(ZSlotDispImg(QImage)));
            QObject::connect(this->m_process,SIGNAL(ZSigSceneFeatureKeyPoints(QImage)),this->m_disp[1],SLOT(ZSlotDispImg(QImage)));
        }else{
            //X mode.
            QObject::connect(this->m_cap1,SIGNAL(ZSigCapImg(QImage)),this->m_disp[0],SLOT(ZSlotDispImg(QImage)));
            QObject::connect(this->m_cap2,SIGNAL(ZSigCapImg(QImage)),this->m_disp[1],SLOT(ZSlotDispImg(QImage)));
        }

        QObject::connect(this->m_cap1,SIGNAL(ZSigCapImg(QImage)),this->m_process,SLOT(ZSlotGetImg1(QImage)));
        QObject::connect(this->m_cap2,SIGNAL(ZSigCapImg(QImage)),this->m_process,SLOT(ZSlotGetImg2(QImage)));
    }

    //safety thread exit mechanism.
    connect(this->m_cap1,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotSubThreadFinished()));
    connect(this->m_cap2,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotSubThreadFinished()));
    connect(this->m_process,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotSubThreadFinished()));

    //display all log messages on the main windows's text edit.
    connect(this->m_cap1,SIGNAL(ZSigMsg(QString,qint32)),this,SLOT(ZSlotMsg(QString,qint32)));
    connect(this->m_cap2,SIGNAL(ZSigMsg(QString,qint32)),this,SLOT(ZSlotMsg(QString,qint32)));
    connect(this->m_process,SIGNAL(ZSigMsg(QString,qint32)),this,SLOT(ZSlotMsg(QString,qint32)));


    connect(this->m_process,SIGNAL(ZSigDiffXYT(QRect,QRect,qint32,qint32,qint32)),this,SLOT(ZSlotDiffXYT(QRect,QRect,qint32,qint32,qint32)));
    connect(this->m_process,SIGNAL(ZSigSSIMImgSimilarity(qint32)),this,SLOT(ZSlotSSIMImgSimilarity(qint32)));
#if 0
    //we make sure the image1 is bigger than image2.
    qint32 nImage1Size=this->m_cap1->ZGetCAMImgWidth()*this->m_cap1->ZGetCAMImgHeight();
    qint32 nImage2Size=this->m_cap2->ZGetCAMImgWidth()*this->m_cap2->ZGetCAMImgHeight();
    if(nImage1Size>=nImage2Size)
    {
        this->ZSlotMsg(tr("%1 > %2 , use %1 as main camera.")///<
                       .arg(this->m_cap1->ZGetDevName())//<
                       .arg(this->m_cap2->ZGetDevName())///<
                       .arg(this->m_cap1->ZGetDevName()),Log_Msg_Info);

        //Capture thread to ImgProcess thread vector.
        connect(this->m_cap1,SIGNAL(ZSigCapImg(QImage)),this->m_process,SLOT(ZSlotGetImg1(QImage)));
        connect(this->m_cap2,SIGNAL(ZSigCapImg(QImage)),this->m_process,SLOT(ZSlotGetImg2(QImage)));
    }else{
        this->ZSlotMsg(tr("%1 > %2 , use %1 as main camera.")///<
                       .arg(this->m_cap2->ZGetDevName())///<
                       .arg(this->m_cap1->ZGetDevName())///<
                       .arg(this->m_cap2->ZGetDevName()),Log_Msg_Info);
        //Capture thread to ImgProcess thread vector.
        connect(this->m_cap2,SIGNAL(ZSigCapImg(QImage)),this->m_process,SLOT(ZSlotGetImg1(QImage)));
        connect(this->m_cap1,SIGNAL(ZSigCapImg(QImage)),this->m_process,SLOT(ZSlotGetImg2(QImage)));
    }
#endif

    //transfer video to pc through UDT.
    if(gGblPara.m_bTransfer2PC)
    {
        this->m_video2PC=new ZServerThread(this->m_process);
        connect(this->m_video2PC,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotSubThreadFinished()));
        this->m_video2PC->ZStartThread();
    }

    //start img process thread.
    this->m_process->ZStartThread();

    //start img capture thread.
    this->m_cap1->ZStartThread();
    this->m_cap2->ZStartThread();
#endif
}

void ZMainTask::ZSlotMsg(const QString &msg,const qint32 &type)
{
    switch(type)
    {
    case Log_Msg_Info:
        qDebug()<<"<info>:"<<msg;
        break;
    case Log_Msg_Warning:
        qDebug()<<"<warning>:"<<msg;
        break;
    case Log_Msg_Error:
        qDebug()<<"<error>:"<<msg;
        break;
    default:
        qDebug()<<"<unknown>:"<<msg;
        break;
    }
}
void ZMainTask::ZSlotSSIMImgSimilarity(qint32 nVal)
{
    this->ZUpdateMatchBar(this->m_SSIMMatchBar,nVal);
}
void ZMainTask::ZSlot1sTimeout()
{
    this->m_nTimeCnt++;
    //convert sec to h/m/s.
    qint32 nReaningSec=this->m_nTimeCnt;
    qint32 nHour,nMinute,nSecond;
    nHour=nReaningSec/3600;
    nReaningSec=nReaningSec%3600;

    nMinute=nReaningSec/60;
    nReaningSec=nReaningSec%60;

    nSecond=nReaningSec;

    this->m_llRunSec->setText(tr("Accumulated\n%1H%2M%3S").arg(nHour).arg(nMinute).arg(nSecond));

    //check global start flag.
    if(gGblPara.m_bGblStartFlag)
    {
        this->m_llState->setText(tr("State\n[Running]"));
    }else{
        this->m_llState->setText(tr("State\n[Stop]"));
    }
}
void ZMainTask::ZUpdateMatchBar(QProgressBar *pBar,qint32 nVal)
{
    pBar->setValue(nVal);
    if(nVal<=100 && nVal>80)
    {
        pBar->setStyleSheet("QProgressBar::chunk{ background-color:#00FF00}");
    }else if(nVal<=80 && nVal>60)
    {
        pBar->setStyleSheet("QProgressBar::chunk{ background-color:#00EE76}");
    }else if(nVal<=60 && nVal>40)
    {
        pBar->setStyleSheet("QProgressBar::chunk{ background-color:#43CD80}");
    }else if(nVal<=40 && nVal>20)
    {
        pBar->setStyleSheet("QProgressBar::chunk{ background-color:#FFAEB9}");
    }else if(nVal<=20 && nVal>=0)
    {
        pBar->setStyleSheet("QProgressBar::chunk{ background-color:#FF0000}");
    }
}
void ZMainTask::ZSlotSubThreadFinished()
{
    QString msgTips("main thread quit while all sub threads exited.");
    //check if sub threads exit okay.
    //then I do exit.
    bool bRun1=this->m_cap1->ZIsRunning();
    bool bRun2=this->m_cap2->ZIsRunning();
    bool bRun3=this->m_process->ZIsRunning();
    if(gGblPara.m_bTransfer2PC)
    {
        if(!bRun1 && !bRun2 && !bRun3)
        {
            this->ZSlotMsg(msgTips,Log_Msg_Info);
            qApp->exit(0);
        }
    }else{
        if(!bRun1 && !bRun2 && !bRun3)
        {
            this->ZSlotMsg(msgTips,Log_Msg_Info);
            qApp->exit(0);
        }
    }
}
void ZMainTask::ZSlotDispatchProcessedSet()
{
    //fetch processed set from processedSet queue.
    //and dispatch them to local two displayer.
    if(this->m_semaProcessedSetUsed->tryAcquire())//已用信号量减1.
    {
        ZImgProcessedSet processSet;
        processSet=this->m_queueProcessedSet->dequeue();
        this->m_semaProcessedSetFree->release();//空闲信号量加1.

        this->m_disp[0]->ZSetSensitiveRect(processSet.rectTemplate);
        this->m_disp[1]->ZSetSensitiveRect(processSet.rectMatched);
        this->m_llDiffXY->setText(tr("Diff XYT\n[X:%1 Y:%2 T:%3ms]").arg(processSet.nDiffX).arg(processSet.nDiffY).arg(processSet.nCostMs));
    }
}
