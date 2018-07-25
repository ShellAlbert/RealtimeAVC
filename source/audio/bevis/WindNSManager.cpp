#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "WindNSManager.h"
#include <QDebug>
static void	 AnalysisWindowFunction(Int32 frmSize, Int32 frmShift, Float winBuf[])
{
    Int32 n;
    for(n=0; n<frmSize; n++)
        winBuf[n] = 1;
    for(n=0; n<frmShift; n++)
    {
        winBuf[n] = (Float) sqrt(0.5 * (1.0 - cos(TWO_PI*0.5 * (n+1.0)/frmShift)));
        winBuf[frmSize-1-n] = winBuf[n];
    }
}

_WINDNSManager::_WINDNSManager()
{

}

_WINDNSManager::~_WINDNSManager()
{

}

void _WINDNSManager::vp_init()
{
    AnalysisWindowFunction(FRAME_LEN,FRAME_SHT,AnalysisWnd);
    memset(xinp,0,sizeof(Float)*2*FRAME_LEN);
    memset(yinp,0,sizeof(Float)*2*FRAME_LEN);
    memset(wxinp,0,sizeof(Float)*2*FRAME_LEN);
    memset(Xkr,0,sizeof(Float)*2*FRAME_LEN);
    memset(Xkr_pre,0,sizeof(Float)*4*FRAME_LEN);
    memset(Xki_pre,0,sizeof(Float)*4*FRAME_LEN);
    memset(Xki,0,sizeof(Float)*2*FRAME_LEN);
    memset(Ykr,0,sizeof(Float)*2*FRAME_LEN);
    memset(Yki,0,sizeof(Float)*2*FRAME_LEN);
    memset(Cohpxx,0,sizeof(Float)*FRAME_LEN);
    memset(Cohpyy,0,sizeof(Float)*FRAME_LEN);
    memset(Cohpxyr,0,sizeof(Float)*FRAME_LEN);
    memset(Cohpxyi,0,sizeof(Float)*FRAME_LEN);
    memset(Gain,0,sizeof(Float)*FRAME_LEN);
    mode = 0;
    frmcount = 0;
    maxRxx = 0.0;
    maxRyy = 0.0;
}

void _WINDNSManager::vp_uninit()
{

}
//设置降噪等级,0:无，1/2/3/4级降噪.
void _WINDNSManager::vp_setwindnsmode(Int16 val)
{
    mode = val;
    return;
}

Int16 _WINDNSManager::vp_getwindnsmode()
{
    return mode;
}

void _WINDNSManager::vp_process( Int16 *xfrm, Int16 *yfrm, Int16 ndims, Int16 len)
{
    Int16 micIdx,samIdx,binIdx;
    Float alpha,tmp,tmp1,tmp2;
    Int16 fftLen = FRAME_LEN;
    Int16 frmSht = FRAME_SHT;
    Float Rxyr[FRAME_LEN];
    Float Rxyi[FRAME_LEN];
    Float xenergy,yenergy;
    Float tmpmaxRxx,tmpmaxRyy;
    //维数不为2
    if(ndims != 2)
    {
        qDebug()<<"<Error>:vp_process(),ndims!=2.";
        return;
    }
    //2*256=512.
    //一次仅能处理512字节
    if(len != (ndims*frmSht))
    {
        qDebug()<<"<Error>:vp_process(),len!=2*256";
        return;
    }

    frmcount = frmcount + 1;

    //data buffering
    memcpy(xinp[0],xinp[0]+frmSht,sizeof(Float)*(fftLen-frmSht));
    memcpy(xinp[1],xinp[1]+frmSht,sizeof(Float)*(fftLen-frmSht));

    //将数据转换后从输入xfrm存入内部缓存
    for(samIdx=0; samIdx<frmSht;samIdx++)
    {
        xinp[0][fftLen-frmSht+samIdx] = (Float)xfrm[2*samIdx+0]/32768.0;
        xinp[1][fftLen-frmSht+samIdx] = (Float)xfrm[2*samIdx+1]/32768.0;
    }

    //windowing
    for(samIdx=0; samIdx<fftLen; samIdx++)
    {
        wxinp[0][samIdx] = xinp[0][samIdx]*AnalysisWnd[samIdx];
        wxinp[1][samIdx] = xinp[1][samIdx]*AnalysisWnd[samIdx];
    }

    //prepare data and fast Fourier transform
    for(micIdx=0; micIdx<ndims; micIdx++)
    {
        memcpy(Xkr[micIdx],wxinp[micIdx],sizeof(Float)*fftLen);
        memset(Xki[micIdx],0,sizeof(Float)*fftLen);
        FFT(Xkr[micIdx],Xki[micIdx],fftLen);
    }

    if(frmcount == 1){
        alpha = 0.8;
    }else{
        alpha = Gain[9];
        for(binIdx=10; binIdx<129; binIdx++)
            alpha = alpha + Gain[binIdx];
        alpha = alpha/120;
        alpha = _CMIN(_CMAX(alpha,0.6),0.9);
    }

    //calculate correlation
    xenergy = 0.0;	yenergy = 0.0;
    memset(Rxyr,0,sizeof(Float)*fftLen);
    memset(Rxyi,0,sizeof(Float)*fftLen);
    for(binIdx=1; binIdx<fftLen; binIdx++){
        Rxyr[binIdx] = Xkr_pre[0][binIdx]*Xkr[0][binIdx] + Xki_pre[0][binIdx]*Xki[0][binIdx];
        Rxyi[binIdx] = Xki_pre[0][binIdx]*Xkr[0][binIdx] - Xkr_pre[0][binIdx]*Xki[0][binIdx];

        xenergy = xenergy + Xkr_pre[0][binIdx]*Xkr_pre[0][binIdx] + Xki_pre[0][binIdx]*Xki_pre[0][binIdx];
        yenergy = yenergy + Xkr[0][binIdx]*Xkr[0][binIdx] + Xki[0][binIdx]*Xki[0][binIdx];
    }
    xenergy = xenergy/(fftLen-1);
    yenergy = yenergy/(fftLen-1);

    IFFT(Rxyr,Rxyi,fftLen);
    for(binIdx=0; binIdx<=fftLen/2; binIdx++){
        tmpmaxRxx = Rxyr[binIdx]/sqrt(xenergy*yenergy);
        if(tmpmaxRxx > maxRxx)
            maxRxx = tmpmaxRxx;
    }

    xenergy = 0.0;	yenergy = 0.0;
    memset(Rxyr,0,sizeof(Float)*fftLen);
    memset(Rxyi,0,sizeof(Float)*fftLen);
    for(binIdx=1; binIdx<fftLen; binIdx++){
        Rxyr[binIdx] = Xkr_pre[1][binIdx]*Xkr[1][binIdx] + Xki_pre[1][binIdx]*Xki[1][binIdx];
        Rxyi[binIdx] = Xki_pre[1][binIdx]*Xkr[1][binIdx] - Xkr_pre[1][binIdx]*Xki[1][binIdx];

        xenergy = xenergy + Xkr_pre[1][binIdx]*Xkr_pre[1][binIdx] + Xki_pre[1][binIdx]*Xki_pre[1][binIdx];
        yenergy = yenergy + Xkr[1][binIdx]*Xkr[1][binIdx] + Xki[1][binIdx]*Xki[1][binIdx];
    }
    xenergy = xenergy/(fftLen-1);
    yenergy = yenergy/(fftLen-1);

    IFFT(Rxyr,Rxyi,fftLen);
    for(binIdx=0; binIdx<=fftLen/2; binIdx++){
        tmpmaxRyy = Rxyr[binIdx]/sqrt(xenergy*yenergy);
        if(tmpmaxRyy > maxRyy)
            maxRyy = tmpmaxRyy;
    }

    memcpy(Xkr_pre[0],Xkr_pre[2],sizeof(Float)*fftLen);
    memcpy(Xkr_pre[1],Xkr_pre[3],sizeof(Float)*fftLen);
    memcpy(Xkr_pre[2],Xkr[0],sizeof(Float)*fftLen);
    memcpy(Xkr_pre[3],Xkr[1],sizeof(Float)*fftLen);
    memcpy(Xki_pre[0],Xki_pre[2],sizeof(Float)*fftLen);
    memcpy(Xki_pre[1],Xki_pre[3],sizeof(Float)*fftLen);
    memcpy(Xki_pre[2],Xki[0],sizeof(Float)*fftLen);
    memcpy(Xki_pre[3],Xki[1],sizeof(Float)*fftLen);


    //calculate coherence
    if(frmcount==1)
    {
        for(binIdx=0; binIdx<=fftLen/2; binIdx++)
        {
            Cohpxx[binIdx]	= Xkr[0][binIdx]*Xkr[0][binIdx] + Xki[0][binIdx]*Xki[0][binIdx];
            Cohpyy[binIdx]	= Xkr[1][binIdx]*Xkr[1][binIdx] + Xki[1][binIdx]*Xki[1][binIdx];
            Cohpxyr[binIdx]	= Xkr[0][binIdx]*Xkr[1][binIdx] + Xki[0][binIdx]*Xki[1][binIdx];
            Cohpxyi[binIdx]	= Xkr[0][binIdx]*Xki[1][binIdx] - Xki[0][binIdx]*Xkr[1][binIdx];
        }
    }else{
        for(binIdx=0; binIdx<=fftLen/2; binIdx++)
        {
            tmp = Xkr[0][binIdx]*Xkr[0][binIdx] + Xki[0][binIdx]*Xki[0][binIdx];
            Cohpxx[binIdx]	= alpha*Cohpxx[binIdx] + (1-alpha)*tmp;

            tmp = Xkr[1][binIdx]*Xkr[1][binIdx] + Xki[1][binIdx]*Xki[1][binIdx];
            Cohpyy[binIdx]	= alpha*Cohpyy[binIdx] + (1-alpha)*tmp;

            tmp = Xkr[0][binIdx]*Xkr[1][binIdx] + Xki[0][binIdx]*Xki[1][binIdx];
            Cohpxyr[binIdx]	= alpha*Cohpxyr[binIdx] + (1-alpha)*tmp;

            tmp = Xkr[0][binIdx]*Xki[1][binIdx] - Xki[0][binIdx]*Xkr[1][binIdx];
            Cohpxyi[binIdx]	= alpha*Cohpxyi[binIdx] + (1-alpha)*tmp;
        }
    }

    for(binIdx=0; binIdx<fftLen/4; binIdx++)
    {
        tmp1 = Cohpxyr[binIdx]*Cohpxyr[binIdx] + Cohpxyi[binIdx]*Cohpxyi[binIdx];
        tmp2 = _CMAX(Cohpxx[binIdx]*Cohpyy[binIdx],eps);
        tmp  = tmp1/tmp2;

        //设置降噪等级,0:无，1/2/3/4级降噪.
        Gain[binIdx] = 1.0;
        if(mode==1)
            Gain[binIdx] = _CMAX(sqrt(tmp),0.2);
        if(mode==2)
            Gain[binIdx] = _CMAX(tmp,0.2);
        if(mode==3)
            Gain[binIdx] = _CMAX(tmp*tmp,0.2);
        if(mode==4)
            Gain[binIdx] = _CMAX(tmp*tmp*tmp,0.2);
    }
    for(binIdx=fftLen/4; binIdx<=fftLen/2; binIdx++)
    {
        Gain[binIdx] = 1.0;
        if(mode==1)
            Gain[binIdx] = 0.50;
        if(mode==2)
            Gain[binIdx] = 0.25;
        if(mode==3)
            Gain[binIdx] = 0.125;
        if(mode==4)
            Gain[binIdx] = 0.0625;
    }

    for(binIdx=fftLen/2+1; binIdx<fftLen; binIdx++)
        Gain[binIdx] = Gain[fftLen-binIdx];

    for(binIdx=0; binIdx<fftLen; binIdx++)
    {
        Ykr[0][binIdx] = Gain[binIdx]*Xkr[0][binIdx];
        Yki[0][binIdx] = Gain[binIdx]*Xki[0][binIdx];
        Ykr[1][binIdx] = Gain[binIdx]*Xkr[1][binIdx];
        Yki[1][binIdx] = Gain[binIdx]*Xki[1][binIdx];
    }
    IFFT(Ykr[0],Yki[0],fftLen);
    IFFT(Ykr[1],Yki[1],fftLen);

    if(maxRxx > maxRyy){
        //将结果输出到yfrm内存中
        for(samIdx=0; samIdx<frmSht; samIdx++)
        {
            tmp = Ykr[0][samIdx]*AnalysisWnd[samIdx];
            yfrm[2*samIdx+0] = (short)((tmp + yinp[0][fftLen-frmSht+samIdx])*32768.0);
            yinp[0][samIdx] = tmp;

            yfrm[2*samIdx+1] = yfrm[2*samIdx+0];

        }
    }else{
        //将结果输出到yfrm内存中
        for(samIdx=0; samIdx<frmSht; samIdx++)
        {
            tmp = Ykr[1][samIdx]*AnalysisWnd[samIdx];
            yfrm[2*samIdx+1] = (short)((tmp + yinp[1][fftLen-frmSht+samIdx])*32768.0);
            yinp[1][samIdx] = tmp;

            yfrm[2*samIdx+0] = yfrm[2*samIdx+1];
        }
    }

    //for(samIdx=0; samIdx<frmSht; samIdx++)
    //{
    //	tmp = Ykr[0][samIdx]*AnalysisWnd[samIdx];
    //	yfrm[2*samIdx+0] = (short)((tmp + yinp[0][fftLen-frmSht+samIdx])*32768.0);
    //	yinp[0][samIdx] = tmp;

    //	tmp = Ykr[1][samIdx]*AnalysisWnd[samIdx];
    //	yfrm[2*samIdx+1] = (short)((tmp + yinp[1][fftLen-frmSht+samIdx])*32768.0);
    //	yinp[1][samIdx] = tmp;
    //}

    for(samIdx=frmSht; samIdx<fftLen; samIdx++)
    {
        yinp[0][samIdx] = Ykr[0][samIdx]*AnalysisWnd[samIdx];
        yinp[1][samIdx] = Ykr[1][samIdx]*AnalysisWnd[samIdx];
    }
}


