#include "zgblpara.h"
#include <QDebug>
#include <QDateTime>
#include <QSettings>
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <signal.h>
#include <audio/zaudiotask.h>
#include <video/zvideotask.h>
#include <forward/ztcp2uartforwardthread.h>
#include <zavui.h>
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
    QApplication app(argc,argv);
     ///////////////////////////////////////////////////////
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
    QCommandLineOption opVerbose("verbose","enable verbose mode.");
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
    cmdLineParser.addOption(opVerbose);
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
    if(cmdLineParser.isSet(opVerbose))
    {
        gGblPara.m_bVerbose=true;
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

    //write pid to file.
    if(gGblPara.writePid2File()<0)
    {
        return -1;
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

    //install signal handler.
    //Set the signal callback for Ctrl-C
    signal(SIGINT,gSIGHandler);

    //write pid to file.
    QFile filePID("/tmp/AVLizard.pid");
    if(!filePID.open(QIODevice::WriteOnly))
    {
        qDebug()<<"<error>:error to write pid file."<<filePID.errorString();
        return -1;
    }
    char pidBuffer[32];
    memset(pidBuffer,0,sizeof(pidBuffer));
    sprintf(pidBuffer,"%d",getpid());
    filePID.write(pidBuffer,strlen(pidBuffer));
    filePID.close();
    qDebug()<<"write pid to /tmp/AVLizard.pid,"<<getpid()<<".";

    //Main CaptureThread put QImage to this queue.
    //Main View Display UI get QImage from this queue for local display.
    QQueue<QImage> *queueMainDisp=new QQueue<QImage>;
    QSemaphore *semaMainDispUsed=new QSemaphore(0);
    QSemaphore *semaMainDispFree=new QSemaphore(30);

    //Aux CaptureThread put QImage to this queue.
    //Aux View Display UI get QImage from this queue for local display.
    QQueue<QImage> *queueAuxDisp=new QQueue<QImage>;
    QSemaphore *semaAuxDispUsed=new QSemaphore(0);
    QSemaphore *semaAuxDispFree=new QSemaphore(30);

    //queue for connect VideoTask/ImgProcessThread and AVUI/LocalDisplay.
    QQueue<ZImgProcessedSet> *queueProcessedSet=new QQueue<ZImgProcessedSet>;
    QSemaphore *semaProcessedSetUsed=new QSemaphore(0);
    QSemaphore *semaProcessedSetFree=new QSemaphore(30);

    //本地音频波形绘制队列,采集的原始波形.
    QQueue<QByteArray> *queueWavBefore=new QQueue<QByteArray>;
    QSemaphore *semaUsedWavBefore=new QSemaphore(0);
    QSemaphore *semaFreeWavBefore=new QSemaphore(30);

    //经过降噪算法处理过的数据波形.
    QQueue<QByteArray> *queueWavAfter=new QQueue<QByteArray>;
    QSemaphore *semaUsedWavAfter=new QSemaphore(0);
    QSemaphore *semaFreeWavAfter=new QSemaphore(30);


    //start Android(tcp) <--> STM32(uart) forward task.
    ZTcp2UartForwardThread tcp2UartForward;
    tcp2UartForward.ZStartThread();

    //audio task.
    ZAudioTask taskAudio;
    taskAudio.ZBindWaveFormQueueBefore(queueWavBefore,semaUsedWavBefore,semaFreeWavBefore);
    taskAudio.ZBindWaveFormQueueAfter(queueWavAfter,semaUsedWavAfter,semaFreeWavAfter);
    if(taskAudio.ZStartTask()<0)
    {
        if(gGblPara.m_bVerbose)
        {
            qDebug()<<"<error>:failed to start audio task.";
        }
        return -1;
    }

    //video task.
    ZVideoTask taskVideo;
    taskVideo.ZBindMainDispQueue(queueMainDisp,semaMainDispUsed,semaMainDispFree);
    taskVideo.ZBindAuxDispQueue(queueAuxDisp,semaAuxDispUsed,semaAuxDispFree);
    taskVideo.ZBindImgProcessedSet(queueProcessedSet,semaProcessedSetUsed,semaProcessedSetFree);
    if(taskVideo.ZStartTask()<0)
    {
        if(gGblPara.m_bVerbose)
        {
            qDebug()<<"<error>:failed to start video task!";
        }
        return -1;
    }

    //AV UI.
    ZAVUI avUI;
    avUI.ZBindMainDispQueue(queueMainDisp,semaMainDispUsed,semaMainDispFree);
    avUI.ZBindAuxDispQueue(queueAuxDisp,semaAuxDispUsed,semaAuxDispFree);
    avUI.ZBindImgProcessedSet(queueProcessedSet,semaProcessedSetUsed,semaProcessedSetFree);
    avUI.ZBindWaveFormQueueBefore(queueWavBefore,semaUsedWavBefore,semaFreeWavBefore);
    avUI.ZBindWaveFormQueueAfter(queueWavAfter,semaUsedWavAfter,semaFreeWavAfter);
    avUI.showMaximized();

    return app.exec();
}
