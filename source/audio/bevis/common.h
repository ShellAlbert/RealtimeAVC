
/* Written by Bevis, 2011-06-10 */

#ifndef _COMMON_H_
#define _COMMON_H_

typedef double    Float;
typedef short     Int16;
typedef int       Int32;
typedef int       Bool;
//typedef signed _int64 Int64;

#ifndef FALSE
	#define FALSE 0
#endif
#ifndef TRUE
	#define TRUE 1
#endif
#ifndef NULL
	#define NULL	0
#endif

#define  _CMIN(x,y)           ((x)<(y)? (x):(y)) 			    
#define  _CMAX(x,y)           ((x)>(y)? (x):(y)) 			     
#define  _CSQUARE(x)	      ((x)*(x))
#define  _CBOUND(x,L,H)	      _CMAX(L, _CMIN(H, x))			    
#define  _CBOUND_(x)          _CMAX(DATA_MIN, _CMIN(DATA_MAX, x))
#define  _CIN_RANGE(x,L,H)	  ((x)>=(L) && (x)<=(H))
#define  _CABS(x)	          ((x)<(0)? (-x):(x))

#define  EPS                  (1e-9)
#define  FLOAT_MAX            (1e+38)
#define  TWO_PI               ((Float)(2*3.1415926535897932))
#define  PI					  3.1415926535897932

#define  SOUND_SPEED          343
#define  eps                  2.2204e-16

struct _NotchParamter{
	Int32 notchIndex;
	Float notchFrequency;
	Int32 notchGainlevel;
	_NotchParamter *next;
};

#endif
