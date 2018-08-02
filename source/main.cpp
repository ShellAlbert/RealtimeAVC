#include "zgblpara.h"
#include <QDebug>
#include <QDateTime>
#include <QSettings>
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <signal.h>
#include <QMetaType>
#include "zmaintask.h"
Q_DECLARE_METATYPE(ZImgProcessedSet)

void gSIGHandler(int sigNo)
{
    switch(sigNo)
    {
    case SIGINT:
    case SIGKILL:
    case SIGTERM:
        qDebug()<<"prepare to exit...";
        gGblPara.m_bGblRst2Exit=true;
        break;
    default:
        break;
    }
}


//AppName: AVLizard
//capture audio with ALSA,encode with opus.
//capture video with V4L2,encode with h264.
int main(int argc,char **argv)
{
    //运行时先使用--DoNotCmpCamId来跳开USB CamId比对代码
    //这样就会在运行目录下生成cam info文件，从里面找出usb bus info信息
    //将这个bus info字符串复制到AVLizard.ini的id中，用于区别主从摄像头
    //usb-fe3c0000.usb-1
    //usb-fe380000.usb-1

    qRegisterMetaType<ZImgProcessedSet>("ZImgProcessedSet");
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");

    QApplication app(argc,argv);
    //parse audio command line arguments.
    QCommandLineOption opMode("mode","0:capture to file,1:realtime process,default is 1.","runMode","1");
    QCommandLineOption opInRate("inRate","specified the capture input sample rate,default is 48000.","inRate","48000");
    QCommandLineOption opOutRate("outRate","specified the playback output sample rate,default is 48000.","outRate","48000");
    QCommandLineOption opChannels("channels","specified the channel number for Capture & Playback,1 for Mono,2 for Stereo,default is 2.","channels","2");
    QCommandLineOption opDeNoise("denoise","de-noise ctrl,0:off,1:RNNoise,2:WebRTC,3:Bevis,default is 0.","denoise","0");
    QCommandLineOption opGaindB("gaindB","compression gain dB range [0,90],default is 0.","gaindB","0");
    QCommandLineOption opBevisGrade("grade","Bevis:noise reduction grade,range:1~4,default is 1.","grade","1");
    //parse video command line arguments.
    QCommandLineOption opDebug("debug","enable debug mode,output more messages.");
    QCommandLineOption opDoNotCmpCamId("DoNotCmpCamId","do not compare camera IDs,used to get cam info at first time run.");
    QCommandLineOption opDumpCamInfo("camInfo","dump camera parameters to file then exit.");
    QCommandLineOption opCapture("capLog","print capture logs.");
    QCommandLineOption opTransfer("tx2PC","enable transfer h264 stream to PC.");
    QCommandLineOption opSpeed("speed","monitor transfer speed.");//-s transfer speed monitor.
    QCommandLineOption opUART("diffXYT","ouput diff XYT to UART.(x,y) and cost time.");//-y hex uart data.
    QCommandLineOption opXMode("xMode","enable xMode to matchTemplate.");//-x xMode,resize image to 1/2 before matchTemplate.
    QCommandLineOption opFMode("fMode","enable fMode to matchTemplate.");//-f fMode,use feature extractor.
    ///////////////////////////////////////////////////////////////
    QCommandLineOption opHelp("help","print this help message.");//-h help.
    QCommandLineParser cmdLineParser;
    //audio.
    cmdLineParser.addOption(opMode);
    cmdLineParser.addOption(opInRate);
    cmdLineParser.addOption(opOutRate);
    cmdLineParser.addOption(opChannels);
    cmdLineParser.addOption(opDeNoise);
    cmdLineParser.addOption(opGaindB);
    cmdLineParser.addOption(opBevisGrade);
    //video.
    cmdLineParser.addOption(opDebug);
    cmdLineParser.addOption(opDoNotCmpCamId);
    cmdLineParser.addOption(opDumpCamInfo);
    cmdLineParser.addOption(opCapture);
    cmdLineParser.addOption(opTransfer);
    cmdLineParser.addOption(opSpeed);
    cmdLineParser.addOption(opUART);
    cmdLineParser.addOption(opXMode);
    cmdLineParser.addOption(opFMode);
    cmdLineParser.addOption(opHelp);
    cmdLineParser.process(app);

    /////////////////////////////
    if(cmdLineParser.isSet(opHelp))
    {
        cmdLineParser.showHelp(0);
        return 0;
    }
    //audio process.
    if(cmdLineParser.isSet(opMode))
    {
        gGblPara.m_audio.m_runMode=cmdLineParser.value("mode").toInt();
    }
    if(cmdLineParser.isSet(opInRate))
    {
        gGblPara.m_audio.m_nCaptureSampleRate=cmdLineParser.value("inRate").toInt();
    }
    if(cmdLineParser.isSet(opOutRate))
    {
        gGblPara.m_audio.m_nPlaybackSampleRate=cmdLineParser.value("outRate").toInt();
    }
    if(cmdLineParser.isSet(opChannels))
    {
        gGblPara.m_audio.m_nChannelNum=cmdLineParser.value("channels").toInt();
    }
    if(cmdLineParser.isSet(opDeNoise))
    {
        gGblPara.m_audio.m_nDeNoiseMethod=cmdLineParser.value("denoise").toInt();
    }
    if(cmdLineParser.isSet(opGaindB))
    {
        gGblPara.m_audio.m_nGaindB=cmdLineParser.value("gaindB").toInt();
    }
    if(cmdLineParser.isSet(opBevisGrade))
    {
        gGblPara.m_audio.m_nBevisGrade=cmdLineParser.value("grade").toInt();
    }

    //video process.
    if(cmdLineParser.isSet(opDebug))
    {
        gGblPara.m_bDebugMode=true;
    }
    if(cmdLineParser.isSet(opDoNotCmpCamId))
    {
        gGblPara.m_video.m_bDoNotCmpCamId=true;
    }
    if(cmdLineParser.isSet(opDumpCamInfo))
    {
        gGblPara.m_bDumpCamInfo2File=true;
    }
    if(cmdLineParser.isSet(opCapture))
    {
        gGblPara.m_bCaptureLog=true;
    }
    if(cmdLineParser.isSet(opSpeed))
    {
        gGblPara.m_bTransferSpeedMonitor=true;
    }
    if(cmdLineParser.isSet(opTransfer))
    {
        gGblPara.m_bTransfer2PC=true;
    }
    if(cmdLineParser.isSet(opUART))
    {
        gGblPara.m_bDumpUART=true;
    }
    if(cmdLineParser.isSet(opXMode))
    {
        gGblPara.m_bXMode=true;
    }
    if(cmdLineParser.isSet(opFMode))
    {
        gGblPara.m_bFMode=true;
    }

    //这里作一个优先级判断，若xMode和fMode同时启动，则只启动fMode
    if(gGblPara.m_bXMode && gGblPara.m_bFMode)
    {
        gGblPara.m_bXMode=false;
    }

    //read config file.
    QFile fileIni("AVLizard.ini");
    if(!fileIni.exists())
    {
        qDebug()<<"<error>:AVLizard.ini config file missed!";
        qDebug()<<"<error>:a template file was generated!";
        qDebug()<<"<error>:please modify it by manual and run again!";
        gGblPara.resetCfgFile();
        return 0;
    }
    gGblPara.readCfgFile();

    //这里我们确保设置的2个摄像头的分辨率必须一致！
    if((gGblPara.m_widthCAM1 != gGblPara.m_widthCAM2) ||(gGblPara.m_heightCAM1!=gGblPara.m_heightCAM2))
    {
        qDebug()<<"<Error>:the resolution of two cameras are not same.";
        qDebug()<<"<Error>: CAM1("<<gGblPara.m_widthCAM1<<"*"<<gGblPara.m_heightCAM1<<") CAM2("<<gGblPara.m_widthCAM2<<"*"<<gGblPara.m_heightCAM2<<")";
        return -1;
    }

    ////////////////////////////////////////////////////////
    qDebug()<<"AVLizard Version:"<<APP_VERSION<<" Build on"<<__DATE__<<" "<<__TIME__;
    //print out audio related settings.
    qDebug()<<"############# Audio #############";
    qDebug()<<"Capture:"<<gGblPara.m_audio.m_capCardName<<","<<gGblPara.m_audio.m_nCaptureSampleRate<<",Channels:"<<gGblPara.m_audio.m_nChannelNum;
    qDebug()<<"Playback:"<<gGblPara.m_audio.m_playCardName<<","<<gGblPara.m_audio.m_nPlaybackSampleRate<<",Channels:"<<gGblPara.m_audio.m_nChannelNum;
    switch(gGblPara.m_audio.m_nDeNoiseMethod)
    {
    case 0:
        qDebug()<<"DeNoise:Disabled.";
        break;
    case 1:
        qDebug()<<"DeNoise:RNNoise Enabled.";
        break;
    case 2:
        qDebug()<<"DeNoise:WebRTC Enabled.";
        break;
    case 3:
        qDebug()<<"DeNoise:Bevis Enabled,grade "<<gGblPara.m_audio.m_nBevisGrade<<".";
        break;
    default:
        break;
    }
    if(gGblPara.m_audio.m_nGaindB>0)
    {
        if(!(gGblPara.m_audio.m_nGaindB>=0 && gGblPara.m_audio.m_nGaindB<=90))
        {
            gGblPara.m_audio.m_nGaindB=10;
            qDebug()<<"Error gain dB,fix to 10dB.";
        }
        qDebug()<<"Compression Gain dB:Enalbed,"<<gGblPara.m_audio.m_nGaindB;
    }else{
        qDebug()<<"Compression Gain dB:Disabled.";
    }

    //print out video related settings.
    qDebug()<<"############# Video #############";
    if(gGblPara.m_bXMode)
    {
        qDebug()<<"XMode Enabled.";
    }else if(gGblPara.m_bFMode){
        qDebug()<<"FMode Enabled.";
    }

    //write pid to file.
    if(gGblPara.writePid2File()<0)
    {
        return -1;
    }

    //install signal handler.
    //Set the signal callback for Ctrl-C
    signal(SIGINT,gSIGHandler);

#if 0
    //start Android(tcp) <--> STM32(uart) forward task.
    ZTcp2UartForwardThread *tcp2UartForward=new ZTcp2UartForwardThread;
    tcp2UartForward->ZStartThread();

    //ctl thread for Video/Audio.
    ZCtlThread *ctlThread=new ZCtlThread;
    ctlThread->ZStartThread();

    //audio task.
    ZAudioTask *taskAudio=new ZAudioTask;
    if(taskAudio->ZStartTask()<0)
    {
        qDebug()<<"<Error>:failed to start audio task.";
        return -1;
    }

    //video task.
    ZVideoTask *taskVideo=new ZVideoTask;
    if(taskVideo->ZDoInit()<0)
    {
        qDebug()<<"<Error>:failed to init video task.";
        return -1;
    }
    if(taskVideo->ZStartTask()<0)
    {
        qDebug()<<"<Error>:failed to start video task!";
        return -1;
    }

    //AV UI.
    ZAVUI *avUI=new ZAVUI;

    //use signal-slot event to notify local UI flush.
    QObject::connect(taskVideo->ZGetImgCapThread(0),SIGNAL(ZSigNewImgArrived(QImage)),avUI->ZGetImgDisp(0),SLOT(ZSlotFlushImg(QImage)),Qt::AutoConnection);
    QObject::connect(taskVideo->ZGetImgCapThread(1),SIGNAL(ZSigNewImgArrived(QImage)),avUI->ZGetImgDisp(1),SLOT(ZSlotFlushImg(QImage)),Qt::AutoConnection);

    //use signal-slot event to notify UI to flush new image process set.
    QObject::connect(taskVideo->ZGetImgProcessThread(),SIGNAL(ZSigNewProcessSetArrived(ZImgProcessedSet)),avUI,SLOT(ZSlotFlushProcessedSet(ZImgProcessedSet)),Qt::AutoConnection);
    QObject::connect(taskVideo->ZGetImgProcessThread(),SIGNAL(ZSigSSIMImgSimilarity(qint32)),avUI,SLOT(ZSlotSSIMImgSimilarity(qint32)),Qt::AutoConnection);

    //use signal-slot event to flush wave form.
    QObject::connect(taskAudio->ZGetNoiseCutThread(),SIGNAL(ZSigNewWaveBeforeArrived(QByteArray)),avUI,SLOT(ZSlotFlushWaveBefore(QByteArray)),Qt::AutoConnection);
    QObject::connect(taskAudio->ZGetNoiseCutThread(),SIGNAL(ZSigNewWaveAfterArrived(QByteArray)),avUI,SLOT(ZSlotFlushWaveAfter(QByteArray)),Qt::AutoConnection);

    avUI->showMaximized();
#endif
    ZMainTask *mainTask=new ZMainTask;
    if(mainTask->ZStartTask()<0)
    {
        return -1;
    }

    //enter event-loop.
    int ret=app.exec();

    //free memory.
    delete mainTask;

    return ret;
}
