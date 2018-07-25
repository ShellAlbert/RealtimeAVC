#include "zavui.h"

ZAVUI::ZAVUI(QWidget *parent):QFrame(parent)
{
    //未处理过的PCM数据曲线
    this->m_seriesBefore=new QLineSeries;
    this->m_chartBefore=new QChart;
    this->m_chartBefore->setTitle(tr("Original Waveform"));
    this->m_chartBefore->legend()->hide();
    this->m_chartBefore->addSeries(this->m_seriesBefore);
    this->m_chartBefore->createDefaultAxes();
    this->m_chartBefore->axisX()->setRange(0,XYSeriesIODevice::sampleCount);
    this->m_chartBefore->axisY()->setRange(-2,2);
    this->m_chartBefore->setTheme(QChart::ChartThemeBlueCerulean);

    this->m_chartViewBefore=new QChartView(this->m_chartBefore);
    this->m_chartViewBefore->setRenderHint(QPainter::Antialiasing);
    this->m_deviceBefore=new XYSeriesIODevice(this->m_seriesBefore,this);
    this->m_deviceBefore->open(QIODevice::WriteOnly);


    //经过降噪算法处理过后的PCM数据曲线
    this->m_seriesAfter=new QLineSeries;
    this->m_chartAfter=new QChart;
    this->m_chartAfter->setTitle(tr("Noise Suppression Waveform"));
    this->m_chartAfter->legend()->hide();
    this->m_chartAfter->addSeries(this->m_seriesAfter);
    this->m_chartAfter->createDefaultAxes();
    this->m_chartAfter->axisX()->setRange(0,XYSeriesIODevice::sampleCount);
    this->m_chartAfter->axisY()->setRange(-2,2);
    this->m_chartAfter->setTheme(QChart::ChartThemeBlueCerulean);

    this->m_chartViewAfter=new QChartView(this->m_chartAfter);
    this->m_chartViewAfter->setRenderHint(QPainter::Antialiasing);
    this->m_deviceAfter=new XYSeriesIODevice(this->m_seriesAfter,this);
    this->m_deviceAfter->open(QIODevice::WriteOnly);

    //layout wave form.
    this->m_vLayoutWavForm=new QVBoxLayout;
    this->m_vLayoutWavForm->addWidget(this->m_chartViewBefore);
    this->m_vLayoutWavForm->addWidget(this->m_chartViewAfter);

    ///////////////////////////////////////////////
    this->m_llDiffXY=new QLabel;
    this->m_llDiffXY->setAlignment(Qt::AlignCenter);
    this->m_llDiffXY->setText(tr("Diff XYT\n[X:0 Y:0 T:0ms]"));
    this->m_llRunSec=new QLabel;
    this->m_llRunSec->setAlignment(Qt::AlignCenter);
    this->m_llRunSec->setText(tr("Accumulated\n[0H0M0S]"));
    this->m_llState=new QLabel;
    this->m_llState->setAlignment(Qt::AlignCenter);
    this->m_llState->setText(tr("ImgPro\n[Pause]"));
    //透传数据统计Android(tcp) <--> STM32(uart).
    this->m_llForward=new QLabel;
    this->m_llForward->setAlignment(Qt::AlignCenter);
    this->m_llForward->setText(tr("Forward\n0/0"));
    //音频客户端是否连接/视频客户端是否连接/Tcp2Uart是否连接/控制端是否连接.
    this->m_llAVClients=new QLabel;
    this->m_llAVClients->setAlignment(Qt::AlignCenter);
    this->m_llAVClients->setText(tr("A:X V:X/X\nCTL:X FWD:X"));

    this->m_vLayoutInfo=new QVBoxLayout;
    this->m_vLayoutInfo->addWidget(this->m_llDiffXY);
    this->m_vLayoutInfo->addWidget(this->m_llRunSec);
    this->m_vLayoutInfo->addWidget(this->m_llState);
    this->m_vLayoutInfo->addWidget(this->m_llForward);
    this->m_vLayoutInfo->addWidget(this->m_llAVClients);

#if 1
    this->m_llDiffXY->setStyleSheet("QLabel{background:#FFAF60;font:20px;min-width:100px;}");
    this->m_llRunSec->setStyleSheet("QLabel{background:#9999CC;font:20px;min-width:100px;}");
    this->m_llState->setStyleSheet("QLabel{background:#AFAF61;font:20px;min-width:100px;}");
    this->m_llForward->setStyleSheet("QLabel{background:#0099CC;font:20px;min-width:100px;}");
    this->m_llAVClients->setStyleSheet("QLabel{background:#23AF61;font:20px;min-width:100px;}");
#endif

    //top horizontal layout.
    this->m_hLayoutWavFormAndInfo=new QHBoxLayout;
    this->m_hLayoutWavFormAndInfo->addLayout(this->m_vLayoutWavForm);
    this->m_hLayoutWavFormAndInfo->addLayout(this->m_vLayoutInfo);


    this->m_disp[0]=new ZImgDisplayer(gGblPara.m_calibrateX1,gGblPara.m_calibrateY1,true);//the main camera.
    this->m_disp[1]=new ZImgDisplayer(gGblPara.m_calibrateX2,gGblPara.m_calibrateY2);
    this->m_disp[0]->ZSetPaintParameters(QColor(0,255,0));
    this->m_disp[1]->ZSetPaintParameters(QColor(255,0,0));

    this->m_SSIMMatchBar=new QProgressBar;
    this->m_SSIMMatchBar->setOrientation(Qt::Vertical);
    this->m_SSIMMatchBar->setValue(0);
    this->m_SSIMMatchBar->setRange(0,100);
    this->m_SSIMMatchBar->setStyleSheet("QProgressBar::chunk{background-color:#FF0000}");

    this->m_barDGain=new QProgressBar;
    this->m_barDGain->setOrientation(Qt::Vertical);
    this->m_barDGain->setValue(0);
    this->m_barDGain->setRange(0,90);
    this->m_barDGain->setStyleSheet("QProgressBar::chunk{background-color:#00FF00}");
    this->m_nDGainShadow=0;

    this->m_hLayout=new QHBoxLayout;
    this->m_hLayout->addWidget(this->m_disp[0]);
    this->m_hLayout->addWidget(this->m_barDGain);
    this->m_hLayout->addWidget(this->m_SSIMMatchBar);
    this->m_hLayout->addWidget(this->m_disp[1]);

    //main layout.
    this->m_vLayout=new QVBoxLayout;
    this->m_vLayout->addLayout(this->m_hLayoutWavFormAndInfo);
    this->m_vLayout->addLayout(this->m_hLayout);
    this->setLayout(this->m_vLayout);

    this->setStyleSheet("QFrame{background-color:#84C1FF;}");

    //running time.
    this->m_nTimeCnt=0;
    this->m_timer=new QTimer;
    QObject::connect(this->m_timer,SIGNAL(timeout()),this,SLOT(ZSlot1sTimeout()));
    this->m_timer->start(1000);
}
ZAVUI::~ZAVUI()
{
    this->m_timer->stop();
    delete this->m_timer;
    //wav form.
    delete this->m_seriesBefore;
    delete this->m_deviceBefore;
    delete this->m_chartBefore;
    delete this->m_chartViewBefore;

    delete this->m_seriesAfter;
    delete this->m_deviceAfter;
    delete this->m_chartAfter;
    delete this->m_chartViewAfter;

    delete this->m_vLayoutWavForm;

    //info.
    delete this->m_llDiffXY;
    delete this->m_llRunSec;
    delete this->m_llState;
    delete this->m_llForward;
    delete this->m_llAVClients;
    delete this->m_vLayoutInfo;

    delete this->m_hLayoutWavFormAndInfo;

    delete this->m_barDGain;
    delete this->m_SSIMMatchBar;
    delete this->m_disp[0];
    delete this->m_disp[1];
    delete this->m_hLayout;
    delete this->m_vLayout;
}
void ZAVUI::closeEvent(QCloseEvent *event)
{
    //set global request exit flag.
    gGblPara.m_bGblRst2Exit=true;
    event->ignore();
}
qint32 ZAVUI::ZBindWaveFormQueueBefore(ZRingBuffer *rbWaveBefore)
{
    this->m_rbWaveBefore=rbWaveBefore;
    return 0;
}
qint32 ZAVUI::ZBindWaveFormQueueAfter(ZRingBuffer *rbWaveAfter)
{
    this->m_rbWaveAfter=rbWaveAfter;
    return 0;
}
ZImgDisplayer* ZAVUI::ZGetImgDisp(qint32 index)
{
    return this->m_disp[index];
}
void ZAVUI::ZSlotSSIMImgSimilarity(qint32 nVal)
{
    this->ZUpdateMatchBar(this->m_SSIMMatchBar,nVal);
}
void ZAVUI::ZSlot1sTimeout()
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

    //ImgPro on/off.
    if(gGblPara.m_bJsonImgPro)
    {
        this->m_llState->setText(tr("ImgPro\n[Running]"));
    }else{
        this->m_llState->setText(tr("ImgPro\n[Pause]"));
    }

    //update the andorid(tcp) <--> STM32(uart) bytes.
    this->m_llForward->setText(tr("Forward\n%1/%2").arg(gGblPara.m_nTcp2UartBytes).arg(gGblPara.m_nUart2TcpBytes));

    //update the connected flags.
    QString audioClient("X");
    if(gGblPara.m_audio.m_bAudioTcpConnected)
    {
        audioClient="V";
    }

    QString videoClient("X");
    if(gGblPara.m_bVideoTcpConnected)
    {
        videoClient="V";
    }
    QString videoClient2("X");
    if(gGblPara.m_bVideoTcpConnected2)
    {
        videoClient2="V";
    }

    QString ctlClient("X");
    if(gGblPara.m_bCtlClientConnected)
    {
        ctlClient="V";
    }

    QString forwardClient("X");
    if(gGblPara.m_bTcp2UartConnected)
    {
        forwardClient="V";
    }

    this->m_llAVClients->setText(tr("A:%1 V:%2/%3\nCTL:%4 FWD:%5").arg(audioClient).arg(videoClient).arg(videoClient2).arg(ctlClient).arg(forwardClient));

    //update the DGain bar.
    if(this->m_nDGainShadow!=gGblPara.m_audio.m_nGaindB)
    {
        this->m_barDGain->setValue(gGblPara.m_audio.m_nGaindB);
        this->m_nDGainShadow=gGblPara.m_audio.m_nGaindB;
    }
}
void ZAVUI::ZUpdateMatchBar(QProgressBar *pBar,qint32 nVal)
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
void ZAVUI::ZSlotFlushWaveBefore(const QByteArray &baPCM)
{
    this->m_deviceBefore->write(baPCM);
}
void ZAVUI::ZSlotFlushWaveAfter(const QByteArray &baPCM)
{
    this->m_deviceAfter->write(baPCM);
}
void ZAVUI::ZSlotFlushProcessedSet(const ZImgProcessedSet &imgProSet)
{
    this->m_disp[0]->ZSetSensitiveRect(imgProSet.rectTemplate);
    this->m_disp[1]->ZSetSensitiveRect(imgProSet.rectMatched);
    this->m_llDiffXY->setText(tr("Diff XYT\n[X:%1 Y:%2 T:%3ms]").arg(imgProSet.nDiffX).arg(imgProSet.nDiffY).arg(imgProSet.nCostMs));
}
