#include "zpcmencthread.h"
#include "../zgblpara.h"
#include <QDebug>
#include <opus/opus.h>
#include <opus/opus_multistream.h>
#include <opus/opus_defines.h>
#include <opus/opus_types.h>
#include <QFile>
#include <QApplication>
ZPCMEncThread::ZPCMEncThread()
{
    this->m_bRunning=false;
}
qint32 ZPCMEncThread::ZStartThread(QQueue<QByteArray> *queueEnc,QSemaphore *semaUsedEnc,QSemaphore *semaFreeEnc,///<
                                   QQueue<QByteArray> *queueTCP,QSemaphore *semaUsedTCP,QSemaphore *semaFreeTCP)
{
    this->m_queueEnc=queueEnc;
    this->m_semaUsedEnc=semaUsedEnc;
    this->m_semaFreeEnc=semaFreeEnc;
    ////////////////////////////////////
    this->m_queueTCP=queueTCP;
    this->m_semaUsedTCP=semaUsedTCP;
    this->m_semaFreeTCP=semaFreeTCP;

    /////////////////
    this->start();

    return 0;
}
qint32 ZPCMEncThread::ZStopThread()
{
    this->m_bRunning=true;
    return 0;
}
bool ZPCMEncThread::ZIsRunning()
{
    return this->m_bRunning;
}
void ZPCMEncThread::run()
{
    char *pEncBuffer=new char[BLOCK_SIZE];
    char *pOpusTails=new char[OPUS_BLKFRM_SIZEx2];
    qint32 nOpusTailsLen=0;
    int err;
    OpusMSEncoder *encoder;
    //48KHz sample rate, 2 channels.
    /** Allocates and initializes a multistream encoder state.
      * Call opus_multistream_encoder_destroy() to release
      * this object when finished.

      * @param coupled_streams <tt>int</tt>: Number of coupled (2 channel) streams
      *                                      to encode.
      *                                      This must be no larger than the total
      *                                      number of streams.
      *                                      Additionally, The total number of
      *                                      encoded channels (<code>streams +
      *                                      coupled_streams</code>) must be no
      *                                      more than the number of input channels.
      * @param[in] mapping <code>const unsigned char[channels]</code>: Mapping from
      *                    encoded channels to input channels, as described in
      *                    @ref opus_multistream. As an extra constraint, the
      *                    multistream encoder does not allow encoding coupled
      *                    streams for which one channel is unused since this
      *                    is never a good idea.
      * @param application <tt>int</tt>: The target encoder application.
      *                                  This must be one of the following:
      * <dl>
      * <dt>#OPUS_APPLICATION_VOIP</dt>
      * <dd>Process signal for improved speech intelligibility.</dd>
      * <dt>#OPUS_APPLICATION_AUDIO</dt>
      * <dd>Favor faithfulness to the original input.</dd>
      * <dt>#OPUS_APPLICATION_RESTRICTED_LOWDELAY</dt>
      * <dd>Configure the minimum possible coding delay by disabling certain modes
      * of operation.</dd>
      * </dl>
      * @param[out] error <tt>int *</tt>: Returns #OPUS_OK on success, or an error
      *                                   code (see @ref opus_errorcodes) on
      *                                   failure.
      */
    //1.Sampling rate of the input signal (in Hz).48000.
    //2.Number of channels in the input signal.2 channels.
    //3.The total number of streams to encode from the input.This must be no more than the number of channels. 0.
    //4.
    /*
     *       opus_int32 Fs,
      int channels,
      int mapping_family,
      int *streams,
      int *coupled_streams,
      unsigned char *mapping,
      int application,
      int *error
      */
    int mapping_family=0;
    int streams=1;
    int coupled_streams=1;
    unsigned char stream_map[255];
    encoder=opus_multistream_surround_encoder_create(SAMPLE_RATE,CHANNELS_NUM,mapping_family,&streams,&coupled_streams,stream_map,OPUS_APPLICATION_AUDIO,&err);
    if(err!=OPUS_OK || encoder==NULL)
    {
        qDebug()<<"<error>:error at opus_multistream_surround_encoder_create(),"<<opus_strerror(err);
        return;
    }
#if 0
    encoder=opus_encoder_create(SAMPLE_RATE,CHANNELS_NUM,OPUS_APPLICATION_AUDIO,&err);
    if(err!=OPUS_OK || encoder==NULL)
    {
        qDebug()<<"<error>:error at opus_encoder_create().";
        return;
    }

    if(opus_encoder_ctl(encoder,OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND))!=OPUS_OK)
    {
        qDebug()<<"<error>:error at opus_encoder_ctl().";
        return;
    }
    if(opus_encoder_ctl(encoder,OPUS_SET_BITRATE(OPUS_AUTO))<0)
    {
        qDebug()<<"<error>:error at opus_encoder_ctl().";
        return;
    }
    if(opus_encoder_ctl(encoder,OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE))<0)
    {
        qDebug()<<"<error>:error at opus_encoder_ctl().";
        return;
    }
    if(opus_encoder_ctl(encoder,OPUS_SET_VBR(1))<0)//0:CBR,1:VBR.
    {
        qDebug()<<"<error>:error at opus_encoder_ctl().";
        return;
    }
    if(opus_encoder_ctl(encoder,OPUS_SET_VBR_CONSTRAINT(1))<0)//0:Unconstrained VBR,1:Constrained VBR.
    {
        qDebug()<<"<error>:error at opus_encoder_ctl().";
        return;
    }
    if(opus_encoder_ctl(encoder,OPUS_SET_COMPLEXITY(5))<0)//range:0~10.
    {
        qDebug()<<"<error>:error at opus_encoder_ctl().";
        return;
    }
    if(opus_encoder_ctl(encoder,OPUS_SET_FORCE_CHANNELS(CHANNELS_NUM))<0)//1:Forced Mono,2:Forced Stereo.
    {
        qDebug()<<"<error>:error at opus_encoder_ctl().";
        return;
    }
    if(opus_encoder_ctl(encoder,OPUS_SET_APPLICATION(OPUS_APPLICATION_VOIP))<0)
    {
        qDebug()<<"<error>:error at opus_encoder_ctl().";
        return;
    }
    if(opus_encoder_ctl(encoder,OPUS_SET_INBAND_FEC(1))<0)//0:disabled,1:enabled.
    {
        qDebug()<<"<error>:error at opus_encoder_ctl().";
        return;
    }
    if(opus_encoder_ctl(encoder,OPUS_SET_EXPERT_FRAME_DURATION(OPUS_FRAMESIZE_60_MS))<0)
    {
        qDebug()<<"<error>:error at opus_encoder_ctl().";
        return;
    }
#endif
    /** Encodes an Opus frame.
      * @param [in] st <tt>OpusEncoder*</tt>: Encoder state
      * @param [in] pcm <tt>opus_int16*</tt>: Input signal (interleaved if 2 channels). length is frame_size*channels*sizeof(opus_int16)
      * @param [in] frame_size <tt>int</tt>: Number of samples per channel in the
      *                                      input signal.
      *                                      This must be an Opus frame size for
      *                                      the encoder's sampling rate.
      *                                      For example, at 48 kHz the permitted
      *                                      values are 120, 240, 480, 960, 1920,
      *                                      and 2880.
      *                                      Passing in a duration of less than
      *                                      10 ms (480 samples at 48 kHz) will
      *                                      prevent the encoder from using the LPC
      *                                      or hybrid modes.
      * @param [out] data <tt>unsigned char*</tt>: Output payload.
      *                                            This must contain storage for at
      *                                            least \a max_data_bytes.
      * @param [in] max_data_bytes <tt>opus_int32</tt>: Size of the allocated
      *                                                 memory for the output
      *                                                 payload. This may be
      *                                                 used to impose an upper limit on
      *                                                 the instant bitrate, but should
      *                                                 not be used as the only bitrate
      *                                                 control. Use #OPUS_SET_BITRATE to
      *                                                 control the bitrate.
      * @returns The length of the encoded packet (in bytes) on success or a
      *          negative error code (see @ref opus_errorcodes) on failure.
      */

    qDebug()<<"<MainLoop>:PCMEncThread starts.";
    while(!gGblPara.m_bGblRst2Exit)
    {
        //fetch data from queue.
        QByteArray baPCMData;
        this->m_semaUsedEnc->acquire();//已用信号量减1
        baPCMData=this->m_queueEnc->dequeue();
        this->m_semaFreeEnc->release();//空闲信号量加1
        //encode pcm to opus.
        qint32 nNewPCMBytes=baPCMData.size();
        qint32 nPaddingBytes=0;
        //如果小尾巴缓冲区有数据，则从新数据中复制一段至小尾巴缓冲区，拼成完整一帧，然后编码。
        if(nOpusTailsLen>0)
        {
            //我们还需要多少数据才能拼成一个完整的帧.
            nPaddingBytes=OPUS_BLKFRM_SIZEx2-nOpusTailsLen;
            if(nPaddingBytes>=0)
            {
                //将新的PCM数据复制一段至小尾巴缓冲区拼成一帧.
                memcpy(pOpusTails+nOpusTailsLen,baPCMData.data(),nPaddingBytes);

                //执行编码操作.
                //48000Hz,2 Channels.
                //frame_size is per channel size.so here is 2880.
                //but because we are 2 channels.so the length of pcm data should be 2880*2.
                //frameSize in 16 bit sample here we choose 2880,so the input pcm buffer size is 2880*sizeof(opus_int16) for mono channel.
                //but we are using two channels,so here is 2880*sizeof(opus_int16)*2.
                qint32 nBytes=opus_multistream_encode(encoder,(const opus_int16*)pOpusTails,OPUS_SAMPLE_FRMSIZE,(unsigned char*)pEncBuffer,BLOCK_SIZE);
                //qint32 nBytes=opus_multistream_encode_float(encoder,(const float *)pOpusTails,OPUS_SAMPLE_FRMSIZE,(unsigned char*)pEncBuffer,BLOCK_SIZE);
                if(nBytes<0)
                {
                    qDebug()<<"<error>:error at opus_encode(),"<<opus_strerror(nBytes);
                }else{
                    //qDebug()<<"<opus>:tail encode okay,pcm="<<OPUS_SAMPLE_FRMSIZE<<",ret="<<nBytes;
                    //新的PCM数据量将减少.
                    nNewPCMBytes-=nPaddingBytes;
                    nOpusTailsLen+=nPaddingBytes;

                    //qDebug()<<"we need "<<nPaddingBytes<<",new pcm remaings:"<<nNewPCMBytes;

                    //after encode,fill data to tcp queue.
                    QByteArray newPCMData(pEncBuffer,nBytes);
                    this->m_semaFreeTCP->acquire();//空闲信号量减1
                    this->m_queueTCP->enqueue(newPCMData);
                    this->m_semaUsedTCP->release();//已用信号量加1
                }
            }
            //reset.
            nOpusTailsLen=0;
        }
        //处理完上一次遗留的小尾巴数据了，现在开始正式数据新的PCM数据，但此处要跳过之前已经处理过的数据.
        qint32 nBlocks=nNewPCMBytes/OPUS_BLKFRM_SIZEx2;//(OPUS_BLKFRM_SIZE*CHANNELS_NUM*sizeof(opus_int16));
        qint32 nRemainBytes=nNewPCMBytes%OPUS_BLKFRM_SIZEx2;//(OPUS_BLKFRM_SIZE*CHANNELS_NUM*sizeof(opus_int16));

        //qDebug()<<"blocks="<<nBlocks<<",remaing bytes:"<<nRemainBytes;
        qint32 nOffsetIndex=0;
        for(qint32 i=0;i<nBlocks;i++)
        {
            //Input signal (interleaved if 2 channels). length is frame_size*channels*sizeof(opus_int16).
            //Number of samples per channel in the input signal.
            //
            //pcmData输入数据(双声道即为交错存储模式),长度应该=frame_size*channels*sizeof(opus_int16),采样个数×通道数×16（采样位数）

            //输入音频数据的每个声道的采样数量，这一定是一个opus框架编码器采样率的大小
            //例如，当采样率48KHz时，采样数量允许的数值为120,240,480,960,1920,2880.
            //传递一个持续时间少于10ms的音频数据(480个样本48KHz）编码器将不会使用LPC或混合模式.

            //关于采样率与采样数量的关系
            //opus_encode()/opus_encode_float()在调用时必须使用一帧的音频数据（2.5ms,5ms,10ms,20ms,40ms,60ms).
            //如果采样率为48KHz，则有如下：
            //采样频率Fm=48KHz
            //采样间隔T=1/Fm=1/48000=1/48ms
            //当T0=2.5ms时，则有采样数量N=T0/T=2.5ms/(1/48ms)=120.
            //当T0=5.0ms时，则有采样数量N=T0/T=5.0ms/(1/48ms)=240.
            //当T0=10.0ms时，则有采样数量N=T0/T=10.0ms/(1/48ms)=480.
            //当T0=20.0ms时，则有采样数量N=T0/T=20.0ms/(1/48ms)=960.
            //当T0=40.0ms时，则有采样数量N=T0/T=40.0ms/(1/48ms)=1920.
            //当T0=60.0ms时，则有采样数量N=T0/T=60.0ms/(1/48ms)=2880.

            const opus_int16 *pcmData=(int16_t*)(baPCMData.data()+nPaddingBytes+nOffsetIndex);
            qint32 nBytes=opus_multistream_encode(encoder,pcmData,OPUS_SAMPLE_FRMSIZE,(unsigned char*)pEncBuffer,BLOCK_SIZE);
            //qint32 nBytes=opus_multistream_encode_float(encoder,(const float *)pcmData,OPUS_SAMPLE_FRMSIZE,(unsigned char*)pEncBuffer,BLOCK_SIZE);
            if(nBytes<0)
            {
                qDebug()<<"<error>:error at opus_encode(),"<<opus_strerror(nBytes);
            }else{
                //qDebug()<<"<opus>:encode okay,pcm="<<OPUS_SAMPLE_FRMSIZE<<",opus="<<nBytes;
                //fill data to tcp queue.
                QByteArray newPCMData(pEncBuffer,nBytes);
                this->m_semaFreeTCP->acquire();//空闲信号量减1.
                this->m_queueTCP->enqueue(newPCMData);
                this->m_semaUsedTCP->release();//已用信号量加1.
            }
            nOffsetIndex+=OPUS_BLKFRM_SIZEx2;//OPUS_BLKFRM_SIZE*CHANNELS_NUM*sizeof(opus_int16);
            //qDebug()<<"offsetIndex="<<nOffsetIndex;
        }
        //如果存在遗留字节则当作小尾巴处理，复制到尾巴缓冲区作为下一帧数据的头部数据
        if(nRemainBytes>0)
        {
            char *pTailBytes=(char*)(baPCMData.data()+nPaddingBytes+nOffsetIndex);
            memcpy(pOpusTails,pTailBytes,nRemainBytes);
            nOpusTailsLen=nRemainBytes;
            //qDebug()<<"hold remaing "<<nRemainBytes;
        }
        this->usleep(AUDIO_THREAD_SCHEDULE_US);
    }
    //opus_encoder_destroy(encoder);
    opus_multistream_encoder_destroy(encoder);
    delete [] pOpusTails;
    //set exit flag to help other thread to exit.
    gGblPara.m_audio.m_bPCMEncThreadExitFlag=true;
    emit this->ZSigThreadFinished();
    qDebug()<<"<MainLoop>:PCMEncThread ends.";
    return;
}
