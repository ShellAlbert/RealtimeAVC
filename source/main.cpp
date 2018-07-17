#include "zgblpara.h"
#include <QDebug>
#include <zmaintask.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include "zhistogram.h"
#include <QDateTime>
#include <QSettings>
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <signal.h>
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
    //parse command line arguments.
    QCommandLineOption opDebug("debug","enable debug mode,output more messages.");
    QCommandLineOption opVerbose("verbose","enable verbose mode.");
    QCommandLineOption opDumpCamInfo("camInfo","dump camera parameters to file then exit.");
    QCommandLineOption opCapture("capLog","print capture logs.");
    QCommandLineOption opTransfer("tx2PC","enable transfer h264 stream to PC.");
    QCommandLineOption opSpeed("speed","monitor transfer speed.");//-s transfer speed monitor.
    QCommandLineOption opUART("diffXYT","ouput diff XYT to UART.(x,y) and cost time.");//-y hex uart data.
    QCommandLineOption opXMode("xMode","enable xMode to matchTemplate.");//-x xMode,resize image to 1/2 before matchTemplate.
    QCommandLineOption opFMode("fMode","enable fMode to matchTemplate.");//-f fMode,use feature extractor.
    QCommandLineOption opHelp("help","print this help message.");//-h help.
    QCommandLineParser cmdLineParser;
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

    if(cmdLineParser.isSet(opHelp))
    {
        cmdLineParser.showHelp(0);
    }
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
    if(gGblPara.m_bXMode)
    {
        qDebug()<<"XMode Enabled.";
    }else if(gGblPara.m_bFMode){
        qDebug()<<"FMode Enabled.";
    }
    //install signal handler.
    signal(SIGINT,gSIGHandler);

    ZMainTask task;
    if(task.ZStartTask()<0)
    {
        if(gGblPara.m_bVerbose)
        {
            qDebug()<<"<error>:app exit due to previous error!";
        }
        return -1;
    }
    task.showMaximized();

    return app.exec();
}
