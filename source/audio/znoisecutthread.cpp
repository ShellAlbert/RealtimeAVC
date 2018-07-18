#include "znoisecutthread.h"
#include "../zgblpara.h"
extern "C"
{
#include <rnnoise.h>
}
#include <audio/webrtc/signal_processing_library.h>
#include <audio/webrtc/noise_suppression_x.h>
#include <audio/webrtc/noise_suppression.h>
#include <audio/webrtc/gain_control.h>
#include <audio/bevis/WindNSManager.h>
#include <QDebug>
#include <QFile>

#define FRAME_SIZE_SHIFT 2
#define FRAME_SIZE (120<<FRAME_SIZE_SHIFT)

void f32_to_s16(int16_t *pOut,const float *pIn,size_t sampleCount)
{
    if(pOut==NULL || pIn==NULL)
    {
        return;
    }
    for(size_t i=0;i<sampleCount;i++)
    {
        *pOut++=(short)pIn[i];
    }
}
void s16_to_f32(float *pOut,const int16_t *pIn,size_t sampleCount)
{
    if(pOut==NULL || pIn==NULL)
    {
        return;
    }
    for(size_t i=0;i<sampleCount;i++)
    {
        *pOut++=pIn[i];
    }
}
ZNoiseCutThread::ZNoiseCutThread()
{

}
qint32 ZNoiseCutThread::ZBindWaveFormQueueBefore(QQueue<QByteArray> *queue,QSemaphore *semaUsed,QSemaphore *semaFree)
{
    this->m_queueWavBefore=queue;
    this->m_semaUsedWavBefore=semaUsed;
    this->m_semaFreeWavBefore=semaFree;
    return 0;
}
qint32 ZNoiseCutThread::ZBindWaveFormQueueAfter(QQueue<QByteArray> *queue,QSemaphore *semaUsed,QSemaphore *semaFree)
{
    this->m_queueWavAfter=queue;
    this->m_semaUsedWavAfter=semaUsed;
    this->m_semaFreeWavAfter=semaFree;
    return 0;
}
qint32 ZNoiseCutThread::ZStartThread(QQueue<QByteArray> *queueNoise,QSemaphore *semaUsedNoise,QSemaphore *semaFreeNoise,///<
                                     QQueue<QByteArray> *queueClear,QSemaphore *semaUsedClear,QSemaphore *semaFreeClear,///<
                                     QQueue<QByteArray> *queueEncode,QSemaphore *semaUsedEncode,QSemaphore *semaFreeEncode)
{
    this->m_queueNoise=queueNoise;
    this->m_semaUsedNoise=semaUsedNoise;
    this->m_semaFreeNoise=semaFreeNoise;

    ///////////////////////////////////
    this->m_queueClear=queueClear;
    this->m_semaUsedClear=semaUsedClear;
    this->m_semaFreeClear=semaFreeClear;

    /////////////////////////
    this->m_queueEncode=queueEncode;
    this->m_semaUsedEncode=semaUsedEncode;
    this->m_semaFreeEncode=semaFreeEncode;

    //////////////////////
    this->start();

    return 0;
}
qint32 ZNoiseCutThread::ZStopThread()
{
    return 0;
}

void ZNoiseCutThread::run()
{

    qint32 nPadding=FRAME_LEN-BLOCK_SIZE%FRAME_LEN;
    char *pFilterBuffer=new char[BLOCK_SIZE+nPadding];

    //RNNoise.
    DenoiseState *st;
    st=rnnoise_create();
    if(st==NULL)
    {
        qDebug()<<"<error>: error at rnnoise_create().";
        return;
    }
    //WebRTC.
    NsHandle *pNS_inst=NULL;
    if (0 != WebRtcNs_Create(&pNS_inst))
    {
        printf("Noise_Suppression WebRtcNs_Create err! \n");
        return;
    }
    if (0 !=  WebRtcNs_Init(pNS_inst,32000))
    {
        printf("Noise_Suppression WebRtcNs_Init err! \n");
        return;
    }
    if (0 !=  WebRtcNs_set_policy(pNS_inst,1))
    {
        printf("Noise_Suppression WebRtcNs_set_policy err! \n");
        return;
    }
    //auto gain.
    void *agcHandle = NULL;
    WebRtcAgc_Create(&agcHandle);
    int minLevel = 0;
    int maxLevel = 255;
    //使用自动增益放大的音量比较有限，所以此处使用固定增益模式.
    int agcMode  = kAgcModeFixedDigital;
    WebRtcAgc_Init(agcHandle, minLevel, maxLevel, agcMode,8000);
    //1.compressionGaindB,在Fixed模式下，该值越大声音越大.
    //2.targetLevelDbfs,表示相对于Full Scale的下降值，0表示full scale,该值越小声音越大.
    WebRtcAgc_config_t agcConfig;
    //compressionGaindB.
    //Set the maximum gain the digital compression stage many apply in dB.
    //A higher number corresponds to greater compression.
    //while a value of 0 will leave the signal uncompressed.
    //value range: limited to [0,90].

    //targetLevelDbfs.
    //According the description of webrtc:
    //change of this parameter will set the target peak level of the AGC in dBFs.
    //the conversion is to use positive values.
    //For instance,passing in a value of 3 corresponds to -3 dBFs or a target level 3dB below full-scale.
    //value range: limited to [0,31].

    //limiterEnable.
    //When enabled,the compression stage will hard limit the signal to the target level.
    //otherwise,the signal will be compressed but not limited above the target level.

    agcConfig.compressionGaindB=gGblPara.m_audio.m_nGaindB;
    agcConfig.targetLevelDbfs=3;//dBFs表示相对于full scale的下降值,0表示full scale.3dB below full-scale.
    agcConfig.limiterEnable=1;
    WebRtcAgc_set_config(agcHandle,agcConfig);

    int frameSize=160;//80;
    int len=frameSize*sizeof(short);
    short *pDataIn=(short*)malloc(frameSize*sizeof(short));
    short *pDataOut=(short*)malloc(frameSize*sizeof(short));

    //bevis.
    qint32 nBevisRemaingBytes=0;
    _WINDNSManager bevis;
    bevis.vp_init();
    //de-noise grade:0/1/2/3.
    bevis.vp_setwindnsmode(gGblPara.m_audio.m_nBevisGrade);

    qDebug()<<"<MainLoop>:NoiseSuppressThread starts.";

    char *pRNNoiseBuffer=new char[FRAME_SIZE];
    qint32 nRNNoiseBufLen=0;
#if 0
    QFile fileRNNoise("rnnoise.pcm");
    fileRNNoise.open(QIODevice::WriteOnly);
    qint32 nFrame=0;
#endif
    while(!gGblPara.m_bGblRst2Exit)
    {
        QByteArray baPCMData;
        //fetch data from noise queue.
        this->m_semaUsedNoise->acquire();//已用信号量减1
        baPCMData=this->m_queueNoise->dequeue();
        this->m_semaFreeNoise->release();//空闲信号量加1

        //将未处理过的数据投入before队列，用于本地波形显示.
        if(this->m_semaFreeWavBefore->tryAcquire())
        {
            this->m_queueWavBefore->enqueue(baPCMData);
            this->m_semaUsedWavBefore->release();
        }
#if 0
        //Automatic gain control from WebRTC for all algorithms.
        if(gGblParam.m_nGaindB>0)
        {
            int micLevelIn=0;
            int micLevelOut=0;

            char *pcmData=(char*)baPCMData.data();
            int nProcessedBytes=0;
            int nRemaingBytes=baPCMData.size();
            while(nRemaingBytes>=len)
            {
                //prepare data.
                memcpy(pDataIn,pcmData+nProcessedBytes,len);

                int inMicLevel=micLevelOut;
                int outMicLevel=0;
                uint8_t saturationWarning;
                int nAgcRet = WebRtcAgc_Process(agcHandle,pDataIn,NULL,frameSize,pDataOut,NULL,inMicLevel,&outMicLevel,0,&saturationWarning);
                if(nAgcRet!=0)
                {
                    qDebug()<<"<error>:error at WebRtcAgc_Process().";
                    break;
                }
                micLevelIn=outMicLevel;
                //copy data out.
                memcpy(pcmData+nProcessedBytes,pDataOut,len);

                //update the processed and remaing bytes.
                nProcessedBytes+=len;
                nRemaingBytes-=len;
            }
        }
#endif

        //de-noise processing.
        if(0==gGblPara.m_audio.m_nDeNoiseMethod)
        {
            //qDebug()<<"DeNoise:disabled";
        }else if(1==gGblPara.m_audio.m_nDeNoiseMethod)
        {
            //qDebug()<<"DeNoise:RNNoise Enabled";
            //because original pcm data is 48000 bytes.
            //so here we choose 480/960/xx.
            //to make sure we have no remaing bytes.
            //here FRAME_SIZE=480.
            const int frame_size=FRAME_SIZE;
            float fPcmData[frame_size];

            //two channels pcm data.
            int16_t *sPcmData=(int16_t*)baPCMData.data();
            //QByteArray::size()返回的是字节数，但此处我们要以int16_t作为基本单位，所以要除以sizeof(int16_t).
            uint32_t nPcmDataInt16Len=baPCMData.size()/sizeof(int16_t);
            //计算需要循环处理多少次，以及还余多少字节.
            qint32 frames=nPcmDataInt16Len/frame_size;
            qint32 remainBytes=nPcmDataInt16Len%frame_size;
            //qDebug()<<"pcm size:"<<baPCMData.size()<<",/16="<<nPcmDataInt16Len;
            //qDebug()<<"size="<<FRAME_SIZE<<",frame="<<frames<<",remaingBytes="<<remainBytes;
            for(qint32 i=0;i<frames;i++)
            {
                for(qint32 j=0;j<frame_size;j++)
                {
                    fPcmData[j]=sPcmData[j];
                }
                rnnoise_process_frame(st,fPcmData,fPcmData);
                for(qint32 j=0;j<frame_size;j++)
                {
                    sPcmData[j]=fPcmData[j];
                }
                sPcmData+=frame_size;
            }

#if 0
            fileRNNoise.write(baPCMData);
            nFrame++;
            if(nFrame>=1000)
            {
                fileRNNoise.close();
                qDebug()<<"rnnoise.pcm okay.";
            }
#endif
        }else if(2==gGblPara.m_audio.m_nDeNoiseMethod)
        {
            //qDebug()<<"DeNoise:WebRTC Enabled";
            int i;
            char *pcmData=(char*)baPCMData.data();
            int  filter_state1[6],filter_state12[6];
            int  Synthesis_state1[6],Synthesis_state12[6];

            memset(filter_state1,0,sizeof(filter_state1));
            memset(filter_state12,0,sizeof(filter_state12));
            memset(Synthesis_state1,0,sizeof(Synthesis_state1));
            memset(Synthesis_state12,0,sizeof(Synthesis_state12));

            for(i=0;i<baPCMData.size();i+=640)
            {
                if(baPCMData.size()-i>= 640)
                {
                    short shBufferIn[320] = {0};
                    short shInL[160],shInH[160];
                    short shOutL[160] = {0},shOutH[160] = {0};

                    memcpy(shBufferIn,(char*)(pcmData+i),320*sizeof(short));
                    //首先需要使用滤波函数将音频数据分高低频，以高频和低频的方式传入降噪函数内部
                    WebRtcSpl_AnalysisQMF(shBufferIn,320,shInL,shInH,filter_state1,filter_state12);

                    //将需要降噪的数据以高频和低频传入对应接口，同时需要注意返回数据也是分高频和低频
                    if (0==WebRtcNs_Process(pNS_inst,shInL,shInH,shOutL,shOutH))
                    {
                        short shBufferOut[320];
                        //如果降噪成功，则根据降噪后高频和低频数据传入滤波接口，然后用将返回的数据写入文件
                        WebRtcSpl_SynthesisQMF(shOutL,shOutH,160,shBufferOut,Synthesis_state1,Synthesis_state12);
                        memcpy(pcmData+i,shBufferOut,320*sizeof(short));
                    }
                }
            }
        }else if(3==gGblPara.m_audio.m_nDeNoiseMethod)
        {
            //qDebug()<<"DeNoise:Bevis Enabled";
            //新的数据帧的地址及大小.
            char *pPcmData=baPCMData.data();
            qint32 nPcmBytes=baPCMData.size();
            //如果上一帧有遗留的尾巴数据，则从新帧中复制出部分数据与之前遗留数据拼帧处理.
            if(nBevisRemaingBytes>0 && nBevisRemaingBytes<FRAME_LEN)
            {
                qint32 nNeedBytes=FRAME_LEN-nBevisRemaingBytes;
                memcpy(pFilterBuffer+nBevisRemaingBytes,pPcmData,nNeedBytes);
                nBevisRemaingBytes+=nNeedBytes;

                //update.
                pPcmData+=nNeedBytes;
                nPcmBytes-=nNeedBytes;
            }
            if(nBevisRemaingBytes>=FRAME_LEN)
            {
                int16_t *pSrc=(int16_t*)(pFilterBuffer);
                char pDst[FRAME_LEN];
                bevis.vp_process(pSrc,(int16_t*)pDst,2,FRAME_LEN);

                QByteArray baPCM(pDst,FRAME_LEN);
                this->m_semaFreeClear->acquire();//空闲信号量减1.
                this->m_queueClear->enqueue(baPCM);
                this->m_semaUsedClear->release();//已用信号量加1.

                //reset.
                nBevisRemaingBytes=0;
            }
            //the Bevis algorithm can only process 512 bytes once.
            //so here we split PCM data to multiple blocks.
            //and process each block then combine the result.
            qint32 nBlocks=nPcmBytes/FRAME_LEN;
            qint32 nRemaingBytes=nPcmBytes%FRAME_LEN;
            //qint32 nPadding=FRAME_LEN-nRemaingBytes;
            //qDebug()<<"total:"<<baPCMData.size()<<",blocks:"<<nBlocks<<",remain bytes:"<<nRemaingBytes<<",padding="<<nPadding;
            for(qint32 i=0;i<nBlocks;i++)
            {
                char *pSrc=pPcmData+FRAME_LEN*i;
                char pDst[FRAME_LEN];
                bevis.vp_process((int16_t*)pSrc,(int16_t*)pDst,2,FRAME_LEN);

                QByteArray baPCM(pDst,FRAME_LEN);
                this->m_semaFreeClear->acquire();//空闲信号量减1.
                this->m_queueClear->enqueue(baPCM);
                this->m_semaUsedClear->release();//已用信号量加1.
            }
            if(nRemaingBytes>0)
            {
                char *pSrc=pPcmData+FRAME_LEN*nBlocks;
                memcpy(pFilterBuffer+nBevisRemaingBytes,pSrc,nRemaingBytes);
                nBevisRemaingBytes+=nRemaingBytes;
            }
        }


#if 1
        //Automatic gain control from WebRTC for all algorithms.
        if(gGblPara.m_audio.m_nGaindB>0)
        {
            int micLevelIn=0;
            int micLevelOut=0;

            char *pcmData=(char*)baPCMData.data();
            int nProcessedBytes=0;
            int nRemaingBytes=baPCMData.size();
            while(nRemaingBytes>=len)
            {
                //prepare data.
                memcpy(pDataIn,pcmData+nProcessedBytes,len);

                int inMicLevel=micLevelOut;
                int outMicLevel=0;
                uint8_t saturationWarning;
                int nAgcRet = WebRtcAgc_Process(agcHandle,pDataIn,NULL,frameSize,pDataOut,NULL,inMicLevel,&outMicLevel,0,&saturationWarning);
                if(nAgcRet!=0)
                {
                    qDebug()<<"<error>:error at WebRtcAgc_Process().";
                    break;
                }
                micLevelIn=outMicLevel;
                //copy data out.
                memcpy(pcmData+nProcessedBytes,pDataOut,len);

                //update the processed and remaing bytes.
                nProcessedBytes+=len;
                nRemaingBytes-=len;
            }
        }
#endif
#if 1
        //put data to clear queue.
        this->m_semaFreeClear->acquire();//空闲信号量减1.
        this->m_queueClear->enqueue(baPCMData);
        this->m_semaUsedClear->release();//已用信号量加1.
#endif
#if 1
        //put data into encode queue.
        //当有客户端连接上时，才将采集到的数据放到编码队列，否则并不投入数据.
        if(gGblPara.m_bTcpClientConnected)
        {
            this->m_semaFreeEncode->acquire();//空闲信号量减1
            this->m_queueEncode->enqueue(baPCMData);
            this->m_semaUsedEncode->release();//已用信号量加1.
        }
#endif
#if 1
        //将未处理过的数据投入after队列，用于本地波形显示.
        if(this->m_semaFreeWavAfter->tryAcquire())
        {
            this->m_queueWavAfter->enqueue(baPCMData);
            this->m_semaUsedWavAfter->release();
        }
#endif
        this->usleep(AUDIO_THREAD_SCHEDULE_US);
    }
    rnnoise_destroy(st);
    WebRtcNs_Free(pNS_inst);
    WebRtcAgc_Free(agcHandle);
    bevis.vp_uninit();
    delete [] pFilterBuffer;
    //set exit flag to help other thread to exit.
    gGblPara.m_audio.m_bCutThreadExitFlag=true;
    emit this->ZSigThreadFinished();
    qDebug()<<"<MainLoop>:NoiseSuppressThread ends.";
    return;
}
