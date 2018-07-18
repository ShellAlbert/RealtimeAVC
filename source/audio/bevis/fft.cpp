/* Written by Bevis, zcai@orpheusys.com, 2011-06-10 */
#include <math.h>
#include "fft.h"

static Int32 fft_table_init[MAX_FFT_TABLE];
static Float fft_sin_table [MAX_FFT_TABLE][MAX_FFT_SIZE];
static Float fft_cos_table [MAX_FFT_TABLE][MAX_FFT_SIZE];


static Int32 FFT_Check(Int32 nFFT)
{	
	Int32 m,i;
	m = MAX_FFT_SIZE;
	for(i=0; i<MAX_FFT_TABLE; i++) 
	{
		if( m == nFFT) break;
		m = m / 2;
	}
	if( i<MAX_FFT_TABLE && fft_table_init[i] == 0)
	{
		for(m=0; m<nFFT/2; m++){
			fft_sin_table[i][m] = (Float)sin(-TWO_PI *m /nFFT);
			fft_cos_table[i][m] = (Float)cos(-TWO_PI *m /nFFT);
		}
		fft_table_init[i] = 1;
	}
	return  i>=MAX_FFT_TABLE ?  -1 : i;
}

Float Hann(int n,int SIZE)
{
	Float w;
	w = 0.5*(1-cos(2*PI*n/(SIZE-1)));
	return w;
}

Bool FFT(Float real[],Float imag[],Int32 nFFT)
{
	Float zreal,zimag,cereal,ceimag;
	Float * sin_table, * cos_table;
	Int32 mr,m,i,n;
	Int32 lay,tp;

	i = FFT_Check(nFFT);
	if (i<0) return 0;

	sin_table = fft_sin_table[i];
	cos_table = fft_cos_table[i];
	n = nFFT;

	mr=0;
	for(m=1; m<n; m++)
	{
		lay=n;
		while(mr+lay>=n)lay=lay>>1;
		mr=mr%lay+lay;
		if(mr<=m)continue;
		zreal=real[m];    zimag=imag[m];   	
		real[m]=real[mr]; imag[m]=imag[mr]; 
		real[mr]=zreal;   imag[mr]=zimag;
	}

	tp = 0, i=n>>1;	while(i>1) tp++, i=i>>1;

	lay=1;	
	while(lay<n){
		for(m=0;m<lay;m++){
			cereal = cos_table[m<<tp], ceimag=sin_table[m<<tp]; /* different sampling rate */
			for(i=m;i<n;i=i+2*lay){		   
				zreal=real[i+lay]*cereal-imag[i+lay]*ceimag;
				zimag=real[i+lay]*ceimag+imag[i+lay]*cereal;
				real[i+lay]=real[i]-zreal;
				imag[i+lay]=imag[i]-zimag;
				real[i]=real[i]+zreal;
				imag[i]=imag[i]+zimag;
			}
		}	
		lay=lay*2;
		tp --;
	}
	/* for(i=0;i<n;i++) imag[i]=-imag[i]; */
	return 1;	
}

Bool IFFT(Float real[],Float imag[],Int32 nFFT)
{
	Int32 i;
	Float scale;

	i = FFT_Check(nFFT);
	if (i<0) return 0;

	FFT(imag,real,nFFT);

	scale = 1.0f/nFFT;
	for(i=0; i<nFFT; i++) {
		real[i] *= scale;
		imag[i]*=scale;
	}
	return 1;
}
