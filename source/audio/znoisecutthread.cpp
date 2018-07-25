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
qint32 ZNoiseCutThread::ZBindWaveFormQueueBefore(ZRingBuffer *rbWave)
{
    this->m_rbWaveBefore=rbWave;
    return 0;
}
qint32 ZNoiseCutThread::ZBindWaveFormQueueAfter(ZRingBuffer *rbWave)
{
    this->m_rbWaveAfter=rbWave;
    return 0;
}
qint32 ZNoiseCutThread::ZStartThread(ZRingBuffer *rbNoise,ZRingBuffer *rbClear,ZRingBuffer *rbEncode)
{
    this->m_rbNoise=rbNoise;
    this->m_rbClear=rbClear;
    this->m_rbEncode=rbEncode;
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
        qDebug()<<"<Error>:NoiseCut,error at rnnoise_create().";
        //set global request to exit flag to cause other threads to exit.
        gGblPara.m_bGblRst2Exit=true;
        return;
    }
    //WebRTC.
    NsHandle *pNS_inst=NULL;
    if(0!=WebRtcNs_Create(&pNS_inst))
    {
        qDebug()<<"<Error>:NoiseCut,error at WebRtcNs_Create().";
        //set global request to exit flag to cause other threads to exit.
        gGblPara.m_bGblRst2Exit=true;
        return;
    }
    if(0!=WebRtcNs_Init(pNS_inst,32000))
    {
        qDebug()<<"<Error>:NoiseCut,error at WebRtcNs_Init().";
        //set global request to exit flag to cause other threads to exit.
        gGblPara.m_bGblRst2Exit=true;
        return;
    }
    if(0!=WebRtcNs_set_policy(pNS_inst,1))
    {
        qDebug()<<"<Error>:NoiseCut,error at WebRtcNs_set_policy().";
        //set global request to exit flag to cause other threads to exit.
        gGblPara.m_bGblRst2Exit=true;
        return;
    }
    //auto gain control.
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

    //这个变量用于控制通过json动态调整增益.
    qint32 nGaindBShadow=gGblPara.m_audio.m_nGaindB;


    //bevis.
    qint32 nBevisRemaingBytes=0;
    _WINDNSManager bevis;
    bevis.vp_init();
    //de-noise grade:0/1/2/3.
    bevis.vp_setwindnsmode(gGblPara.m_audio.m_nBevisGrade);

    qDebug()<<"<MainLoop>:NoiseSuppressThread starts.";

    char *pRNNoiseBuffer=new char[FRAME_SIZE];
    qint32 nRNNoiseBufLen=0;
    //脏音频数据（未经过降噪处理的原始采样数据）.
    QByteArray *baPCMData=new QByteArray(BLOCK_SIZE,0);
    qint32 nPCMDataLen=0;
    while(!gGblPara.m_bGblRst2Exit)
    {
        //fetch noise pcm data from noise queue.
        if(!this->m_rbNoise->m_semaUsed->tryAcquire())
        {
            this->usleep(AUDIO_THREAD_SCHEDULE_US);
            continue;
        }
        nPCMDataLen=this->m_rbNoise->ZGetElement((qint8*)baPCMData->data(),baPCMData->size());
        this->m_rbNoise->m_semaFree->release();
        if(nPCMDataLen<=0)
        {
            qDebug()<<"<Error>:NoiseCut,error length get pcm from noise queue.";
            continue;
        }

        //只有开启本地UI刷新才将采集到的声音数据扔入wavBefore队列.
        //将未处理过的数据投入before队列，用于本地波形显示,超时10ms*10=100ms.
        if(gGblPara.m_bJsonFlushUIWav)
        {
            QByteArray baPCM(baPCMData->data(),nPCMDataLen);
            emit this->ZSigNewWaveBeforeArrived(baPCM);
        }

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
            int16_t *sPcmData=(int16_t*)baPCMData->data();
            //nNoisePCMLen返回的是字节数，但此处我们要以int16_t作为基本单位，所以要除以sizeof(int16_t).
            uint32_t nPcmDataInt16Len=nPCMDataLen/sizeof(int16_t);
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
        }else if(2==gGblPara.m_audio.m_nDeNoiseMethod)
        {
            //qDebug()<<"DeNoise:WebRTC Enabled";
            int i;
            char *pcmData=(char*)baPCMData->data();
            int  filter_state1[6],filter_state12[6];
            int  Synthesis_state1[6],Synthesis_state12[6];

            memset(filter_state1,0,sizeof(filter_state1));
            memset(filter_state12,0,sizeof(filter_state12));
            memset(Synthesis_state1,0,sizeof(Synthesis_state1));
            memset(Synthesis_state12,0,sizeof(Synthesis_state12));

            for(i=0;i<nPCMDataLen;i+=640)
            {
                if(nPCMDataLen-i>= 640)
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
            char *pPcmData=baPCMData->data();
            qint32 nPcmBytes=nPCMDataLen;
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

                //                QByteArray baPCM(pDst,FRAME_LEN);
                //                this->m_semaFreeClear->acquire();//空闲信号量减1.
                //                this->m_queueClear->enqueue(baPCM);
                //                this->m_semaUsedClear->release();//已用信号量加1.

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

                //                QByteArray baPCM(pDst,FRAME_LEN);
                //                this->m_semaFreeClear->acquire();//空闲信号量减1.
                //                this->m_queueClear->enqueue(baPCM);
                //                this->m_semaUsedClear->release();//已用信号量加1.
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
        if(nGaindBShadow!=gGblPara.m_audio.m_nGaindB)
        {
            //如果这2个变量不相等，则动态增益变量肯定被json协议修改了。
            //那么我们重新调整动态增益的相关设置。
            WebRtcAgc_Free(agcHandle);
            agcHandle=NULL;

            //重新初始化.
            WebRtcAgc_Create(&agcHandle);
            WebRtcAgc_Init(agcHandle,0,255,kAgcModeFixedDigital,8000);
            WebRtcAgc_config_t agcConfig;
            agcConfig.compressionGaindB=gGblPara.m_audio.m_nGaindB;
            agcConfig.targetLevelDbfs=3;
            agcConfig.limiterEnable=1;
            WebRtcAgc_set_config(agcHandle,agcConfig);

            qDebug()<<"ctl dgain from "<<nGaindBShadow<<"to"<<gGblPara.m_audio.m_nGaindB;
            //调整完后，使2个变量再次相等。
            nGaindBShadow=gGblPara.m_audio.m_nGaindB;
        }
        if(nGaindBShadow>0)
        {
            int micLevelIn=0;
            int micLevelOut=0;

            char *pcmData=(char*)baPCMData->data();
            int nProcessedBytes=0;
            int nRemaingBytes=nPCMDataLen;
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

        //try to put denoise data to clear queue for local playback,超时10ms*10=100ms.
        qint32 nTryTimes=0;
        do{
            if(this->m_rbClear->m_semaFree->tryAcquire())//空闲信号量减1,这里使用try防止阻塞.
            {
                this->m_rbClear->ZPutElement((qint8*)baPCMData->data(),nPCMDataLen);
                this->m_rbClear->m_semaUsed->release();//已用信号量加1.
                break;
            }
            if(nTryTimes++>10)
            {
                qDebug()<<"<Error>:NoiseCut,timeout to put denoise pcm data to clear queue,trash this frame.";
                break;
            }
            qDebug()<<"<Warning>:NoiseCut clear queue is full,try times "<<nTryTimes<<".";
            this->usleep(AUDIO_THREAD_SCHEDULE_US);
        }while(1);

        //try to put denoise data into encode queue for tcp transfer.
        //当有客户端连接上时，才将采集到的数据放到编码队列，否则并不投入数据,超时10ms*10=100ms.
        if(gGblPara.m_audio.m_bAudioTcpConnected)
        {
            do{
                qint32 nTryTimes=0;
                if(this->m_rbEncode->m_semaFree->tryAcquire())//空闲信号量减1,这里使用try防止阻塞.
                {
                    this->m_rbEncode->ZPutElement((qint8*)baPCMData->data(),nPCMDataLen);
                    this->m_rbEncode->m_semaUsed->release();//已用信号量加1.
                    break;
                }
                if(nTryTimes++>10)
                {
                    qDebug()<<"<Error>:NoiseCut,timeout to put denoise pcm data to encode queue,trash this frame.";
                    break;
                }
                qDebug()<<"<Warning>:NoiseCut encode queue is full,try times "<<nTryTimes<<".";
                this->usleep(AUDIO_THREAD_SCHEDULE_US);
            }while(1);
        }

        //尝试将未处理过的数据投入after队列，用于本地波形显示,超时10ms*10=100ms.
        if(gGblPara.m_bJsonFlushUIWav)
        {
            QByteArray baPCM(baPCMData->data(),nPCMDataLen);
            emit this->ZSigNewWaveAfterArrived(baPCM);
#if 0
            qint32 nTryTimes=0;
            do{
                if(this->m_rbWaveAfter->m_semaFree->tryAcquire())
                {
                    this->m_rbWaveAfter->ZPutElement((qint8*)baPCMData->data(),nPCMDataLen);
                    this->m_rbWaveAfter->m_semaUsed->release();
                    emit this->ZSigNewWaveAfterArrived();
                    break;
                }
                if(nTryTimes++>10)
                {
                    qDebug()<<"<Error>:NoiseCut,timeout to put pcm to wavAfter queue,trash it.";
                    break;
                }
                qDebug()<<"<Warning>:NoiseCut wavAfter queue is full,try times "<<nTryTimes<<".";
                this->usleep(AUDIO_THREAD_SCHEDULE_US);
            }while(1);
#endif
        }
    }
    rnnoise_destroy(st);
    WebRtcNs_Free(pNS_inst);
    WebRtcAgc_Free(agcHandle);
    bevis.vp_uninit();
    delete [] pFilterBuffer;

    qDebug()<<"<MainLoop>:NoiseSuppressThread ends.";
    //set exit flag to help other thread to exit.
    gGblPara.m_audio.m_bCutThreadExitFlag=true;
    //set global request to exit flag to help other thread to exit.
    gGblPara.m_bGblRst2Exit=true;
    emit this->ZSigThreadFinished();

    return;
}
