#include "zh264encthread.h"
#include <zgblpara.h>
#include <QDebug>
#include <QFile>
extern "C"
{
#include <x264.h>
#include <x264_config.h>
}
ZH264EncThread::ZH264EncThread()
{
    this->m_bExitFlag=false;

    this->m_queueYUV=NULL;
    this->m_semaYUVUsed=NULL;
    this->m_semaYUVFree=NULL;

    this->m_queueH264=NULL;
    this->m_semaH264Used=NULL;
    this->m_semaH264Free=NULL;
}
qint32 ZH264EncThread::ZBindYUVQueue(QQueue<QByteArray> *queueYUV,QSemaphore *semaYUVUsed,QSemaphore *semaYUVFree)
{
    this->m_queueYUV=queueYUV;
    this->m_semaYUVUsed=semaYUVUsed;
    this->m_semaYUVFree=semaYUVFree;
    return 0;
}
qint32 ZH264EncThread::ZBindH264Queue(QQueue<QByteArray> *queueH264,QSemaphore *semaH264Used,QSemaphore *semaH264Free)
{
    this->m_queueH264=queueH264;
    this->m_semaH264Used=semaH264Used;
    this->m_semaH264Free=semaH264Free;
    return 0;
}
qint32 ZH264EncThread::ZStartThread()
{
    //check yuv queue.
    if(this->m_queueYUV==NULL || this->m_semaYUVUsed==NULL || this->m_semaYUVFree==NULL)
    {
        qDebug()<<"<error>:no bind yuv queue,cannot start.";
        return -1;
    }
    //check h264 queue.
    if(this->m_queueH264==NULL || this->m_semaH264Used==NULL || this->m_semaH264Free==NULL)
    {
        qDebug()<<"<error>:no bind h264 queue,cannot start.";
        return -1;
    }

    this->start();
    return 0;
}
qint32 ZH264EncThread::ZStopThread()
{
    this->m_bExitFlag=true;
    return 0;
}
#if 0
int yuyv_to_yuv420p(char *in,char *out,int width,int height)
{
    char *p_in,*p_out,*y,*u,*v;
    int index_y,index_u,index_v;
    int i,j,in_len;
    y=out;
    u=out+(width*height);
    v=out+(width*height*5/4);
    index_y=0;
    index_u=0;
    index_v=0;
    for(j=0;j<height*2;j++)
    {
        for(i=0;i<width;i+=4)
        {
            *(y+(index_y++))=*(in+width*j+i);
            *(y+(index_y++))=*(in+width*j+i+2);
            if(j%2==0)
            {
                *(u+(index_u++))=*(in+width*j+i+1);
                *(v+(index_v++))=*(in+width*j+i+3);
            }
        }
    }
    return 0;
}
#else
int yuv422_to_yuv420(const char *in,char *out,int width,int height)
{
    char *y=out;
    char *u=out+width*height;
    char *v=out+width*height+width*height/4;
    //YU YV YU YV
    int yuv422Len=width*height*2;
    int yIndex=0;
    int uIndex=0;
    int vIndex=0;
    int is_u=1;
    for(int i=0;i<yuv422Len;i+=2)
    {
        *(y+(yIndex++))=*(in+i);
    }
    for(int i=0;i<height;i+=2)
    {
        int baseH=i*width*2;
        for(int j=baseH+1;j<baseH+width*2;j+=2)
        {
            if(is_u)
            {
                *(u+(uIndex++))=*(in+j);
                is_u=0;
            }else{
                *(v+(vIndex++))=*(in+j);
                is_u=1;
            }
        }
    }
    return 0;
}
#endif
void ZH264EncThread::run()
{
    int iNal=0;
    x264_t *pHandle=NULL;
    x264_nal_t *pNals=NULL;
    x264_param_t *pParam=(x264_param_t*)malloc(sizeof(x264_param_t));
    x264_picture_t *pPicIn=(x264_picture_t*)malloc(sizeof(x264_picture_t));

    //static const char * const x264_tune_names[] = { "film", "animation", "grain", "stillimage", "psnr", "ssim", "fastdecode", "zerolatency", 0 };
    //    if(x264_param_default_preset(pParam,"zerolatency", NULL)<0)
    //    {
    //        qDebug()<<"<error>: error at x264_param_default_preset().";
    //        return;
    //    }

    //set default parameters.
    //x264_param_default(pParam);
    x264_param_default_preset(pParam,"ultrafast","zerolatency");

    pParam->i_csp=X264_CSP_I420;
    pParam->b_vfr_input = 0;
    pParam->b_repeat_headers = 1;
    pParam->b_annexb = 1;

    pParam->i_threads=X264_SYNC_LOOKAHEAD_AUTO;

    //set width*height.
    pParam->i_width=640;
    pParam->i_height=480;

    pParam->i_frame_total=0;
    pParam->i_keyint_max=10;
    pParam->rc.i_lookahead=0;
    pParam->i_bframe=0;

    pParam->b_open_gop=0;
    pParam->i_bframe_pyramid=0;
    pParam->i_bframe_adaptive=X264_B_ADAPT_TRELLIS;
    pParam->i_fps_num=15;
    pParam->i_fps_den=1;


    //static const char * const x264_profile_names[] = { "baseline", "main", "high", "high10", "high422", "high444", 0 };
    if(x264_param_apply_profile(pParam,x264_profile_names[4])<0)
    {
        qDebug()<<"<error>:error at x264_param_apply_profile().";
        return;
    }

    ////////////////////////////////////////////
    pHandle=x264_encoder_open(pParam);
    if(pHandle==NULL)
    {
        qDebug()<<"<error>:error at x264_encoder_open().";
        return;
    }

    /* yuyv 4:2:2 packed */
    //allocate a picture in.//
    //    pPicIn->img.i_csp=X264_CSP_YUYV;
    //    pPicIn->img.i_plane=1;
    if(x264_picture_alloc(pPicIn,X264_CSP_I420,640,480)<0)
    {
        qDebug()<<"<error>:error at x264_picture_alloc().";
        return;
    }


    qDebug()<<"<MainLoop>:H264EncThread starts.";
    long int pts=0;
    char *pEncodeBuffer=new char[1*1024*1024];

//    QFile fileH264(QString("yantai.h264"));
//    if(fileH264.open(QIODevice::WriteOnly))
//    {

//    }
    char *yuv420pBuffer=new char[1*1024*1024];
    bool bIPBFlag=true;
    while(!gGblPara.m_bGblRst2Exit)
    {
        x264_picture_t picOut;
        int nEncodeBytes=0;

        //1.fetch data from yuv queue.
        QByteArray baImgData;
        this->m_semaYUVUsed->acquire();//已用信号量减1.
        baImgData=this->m_queueYUV->dequeue();
        this->m_semaYUVFree->release();//空闲信号量加1.
        //qDebug()<<"<h264 encode thread> fetch img from yuv queue,size:"<<baImgData.size();
        if(baImgData.size()<=0)
        {
            continue;
        }

        //2.do h264 encode.
        int yuv420Length=640*480*1.5;
        //y=640*480
        //u=640*480*0.25
        //v=640*480*0.25
        yuv422_to_yuv420(baImgData.data(),yuv420pBuffer,640,480);

        char *y=(char*)pPicIn->img.plane[0];
        char *u=(char*)pPicIn->img.plane[1];
        char *v=(char*)pPicIn->img.plane[2];

        memcpy(y,yuv420pBuffer,307200);
        memcpy(u,yuv420pBuffer+307200,76800);
        memcpy(v,yuv420pBuffer+384000,76800);

        if(bIPBFlag)
        {
           pPicIn->i_type=X264_TYPE_KEYFRAME;
        }else{
           pPicIn->i_type=X264_TYPE_P;
        }
        bIPBFlag=!bIPBFlag;
        //pPicIn->i_type=X264_TYPE_P;//
        //pPicIn->i_type=X264_TYPE_IDR;
        //pPicIn->i_type=X264_TYPE_KEYFRAME;
        //pPicIn->i_type=X264_TYPE_B;
        //pPicIn->i_type=X264_TYPE_AUTO;

        //i_pts参数需要递增，否则会出现警告:x264[warning]:non-strictly-monotonic PTS.
        pPicIn->i_pts=pts++;
        //进行编码.
        qint32 ret=x264_encoder_encode(pHandle,&pNals,&iNal,pPicIn,&picOut);
        if(ret<0)
        {
            qDebug()<<"<error>:x264 encode failed.";
        }else{
            for(int j=0;j<iNal;j++)
            {
                /* Size of payload (including any padding) in bytes. */
                //int     i_payload;
                /* If param->b_annexb is set, Annex-B bytestream with startcode.
                 * Otherwise, startcode is replaced with a 4-byte size.
                 * This size is the size used in mp4/similar muxing; it is equal to i_payload-4 */
                //uint8_t *p_payload;
                //fwrite(pNals[j].p_payload,1,pNals[j].i_payload,fp);
                //qDebug()<<"encode size:"<<pNals[j].i_payload;
                memcpy(pEncodeBuffer+nEncodeBytes,pNals[j].p_payload,pNals[j].i_payload);
                nEncodeBytes+=pNals[j].i_payload;
            }

            //            fileH264.write(pEncodeBuffer,nEncodeBytes);
            //            fileH264.flush();
            //flush encode when not all I frames.
#if 0
            while(1)
            {
                if(0==x264_encoder_encode(pHandle,&pNals,&iNal,NULL,&picOut))
                {
                    break;
                }
                for(int j=0;j<iNal;j++)
                {
                    memcpy(pEncodeBuffer+nEncodeBytes,pNals[j].p_payload,pNals[j].i_payload);
                    nEncodeBytes+=pNals[j].i_payload;
                }
            }
#endif
            //qDebug()<<"encode bytes:"<<nEncodeBytes;

            //3.put encode pkt to h264 queue.
            if(nEncodeBytes>0)
            {
                QByteArray baEncodedData(pEncodeBuffer,nEncodeBytes);
                this->m_semaH264Free->acquire();//空闲信号量减1.
                this->m_queueH264->enqueue(baEncodedData);
                this->m_semaH264Used->release();//已用信号量加1.
            }

            this->usleep(1000);//1000us.
        }
    }
    x264_picture_clean(pPicIn);
    x264_encoder_close(pHandle);
    qDebug()<<"<MainLoop>:H264EncThread ends.";
    return;
}
