
#ifndef H_M_CONFIG
#define H_M_CONFIG

#include <stdint.h>

#define SANITY_CHECK

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define ANGLE_MASK 8191
#define ANGLE_1 8192
#define ANGLE_2 4096
#define ANGLE_3 2730
// 3 is not exact
#define ANGLE_4 2048
#define ANGLE_5 1638
// not exact
#define ANGLE_6 1365
#define ANGLE_7 1170
// 6 is not exact
#define ANGLE_8 1024
#define ANGLE_10 819
#define ANGLE_12 683
#define ANGLE_16 512
//#define ANGLE_8_3 384
//#define ANGLE_16_3 192
#define ANGLE_32 256
#define ANGLE_64 128
#define ANGLE_128 64
#define ANGLE_256 32
//#define ANGLE_TO_FIXED 4

#define AFX_MASK 0xffffff

#define AFX_ANGLE_1 al_itofix(256)
#define AFX_ANGLE_2 al_itofix(128)
#define AFX_ANGLE_4 al_itofix(64)
#define AFX_ANGLE_8 al_itofix(32)
#define AFX_ANGLE_16 al_itofix(16)
#define AFX_ANGLE_32 al_itofix(8)
#define AFX_ANGLE_64 al_itofix(4)
#define AFX_ANGLE_128 al_itofix(2)
#define AFX_ANGLE_256 al_itofix(1)

typedef int16_t s16b;
typedef uint16_t u16b;

#ifndef PI
#define PI 3.141592
#endif

#define PI_2 (PI/2)
#define PI_4 (PI/4)
#define PI_8 (PI/8)
#define PI_16 (PI/16)
#define PI_32 (PI/32)

#ifndef MAX
	#define MAX( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef MIN
	#define MIN( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif

#define BCODE_VALUE_MAXIMUM (1<<15)
#define BCODE_VALUE_MINIMUM ((1<<15)*-1)

#define SOURCE_TEXT_LINES 2000
#define SOURCE_TEXT_LINE_LENGTH 160


#ifdef __GNUC__
#define USE_GCC_EXPECT
// Uses GCC's __builtin_expect() for optimising a few things
#endif



#endif


