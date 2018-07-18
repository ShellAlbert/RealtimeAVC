/* Written by Razorenhua,  2011-06-10 */
#ifndef _WINDNSMANAGER_H_
#define _WINDNSMANAGER_H_

#include "common.h"
#include "fft.h"

#define FFT_LEN	512
#define FRAME_LEN 512
#define FRAME_SHT 256


class _WINDNSManager
{
private:
	Float AnalysisWnd[FRAME_LEN];
	Float xinp[2][FRAME_LEN];
	Float yinp[2][FRAME_LEN];
	Float wxinp[2][FRAME_LEN];
	Float Xkr[2][FRAME_LEN];
	Float Xki[2][FRAME_LEN];
	Float Xkr_pre[4][FRAME_LEN];
	Float Xki_pre[4][FRAME_LEN];
	Float Ykr[2][FRAME_LEN];
	Float Yki[2][FRAME_LEN];
	Float Cohpxx[FRAME_LEN];
	Float Cohpyy[FRAME_LEN];
	Float Cohpxyr[FRAME_LEN];
	Float Cohpxyi[FRAME_LEN];
	Float Gain[FRAME_LEN];
	Float maxRxx,maxRyy;
	Int16 mode;
	Int32 frmcount;
public:
	_WINDNSManager();
	~_WINDNSManager();
	
	void vp_init();
	void vp_process(Int16 *xinp, Int16 *yinp, Int16 ndims, Int16 len);
	void vp_uninit();

	void vp_setwindnsmode(Int16 val);
	Int16 vp_getwindnsmode();
};

#endif