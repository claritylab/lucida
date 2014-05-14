/* This is used to customize the utilities to a particular system or machine.  
   You should try to localize your personal machine-specific customizations to
   sysext.h. */

#ifndef SYSTEM_H
#define SYSTEM_H

#include <math.h>


/*************************** BASIC TYPE DEFINITIONS **************************/
/* Boolean values */
/* Don't change these, I use the fact that FALSE is 0 and TRUE is non-zero */
#define FALSE 0
#define TRUE  1

/* Types */
typedef unsigned int mask; /* a type bit field or a mode specifier */
typedef unsigned int flag; /* a boolean.  This is an int because I use
			    Tcl_LinkVar and you can't link to a char */

#ifdef DOUBLE_REAL
  typedef double real;
#else
  typedef float real;
# ifndef FLOAT_REAL
#   define FLOAT_REAL
# endif /* FLOAT_REAL */
#endif /* DOUBLE_REAL */

/*
  NaNf is a 4-byte nan
  NaNd is a 8-byte nan
  NaN is a real nan, whichever that is
  isNaNf tests a 4-byte nan
  isNaNf tests a 8-byte nan
  isNaN tests a real nan, whichever that is
*/

/************************* MACHINE-SPECIFIC SETTINGS *************************/

#ifdef MACHINE_INTEL
#  ifndef LITTLE_END
#    define LITTLE_END
#  endif /* LITTLE_END */
#endif /* MACHINE_INTEL */


#ifdef MACHINE_LINUX
#  include <bits/nan.h>
#  ifndef LITTLE_END
#    define LITTLE_END
#  endif /* LITTLE_END */
#endif /* MACHINE_LINUX */


#ifdef MACHINE_WINDOWS
#  include <sys/cygwin.h>
#  ifndef LITTLE_END
#    define LITTLE_END
#  endif /* LITTLE_END */
#endif /* MACHINE_WINDOWS */


#ifdef MACHINE_MACINTOSH
#  include <sys/types.h>
#  include <machine/types.h>
#  define NO_DRAND48
#  ifdef LITTLE_END
#    undef LITTLE_END
#  endif /* LITTLE_END */
#endif /* MACHINE_MACINTOSH */


#ifdef MACHINE_ALPHA
#  include <float.h>
#  ifndef LITTLE_END
#    define LITTLE_END
#  endif /* LITTLE_END */
#  define isNaNf isnanf

#  ifdef FLOAT_REAL
#    define REAL_MATH
#    define LOG(x)   logf(x)
#    define EXP(x)   expf(x)
#    define SQRT(x)  sqrtf(x)
#    define POW(x,y) powf(x,y)
#    define ABS(x)   fabsf(x)
#    define CEIL(x)  ((int) ceilf(x))
#    define FLOOR(x) ((int) floorf(x))
#  endif /* FLOAT_REAL */
#endif /* MACHINE_ALPHA */


#ifdef MACHINE_SUN4
/* Big endian, nothing needed. */
#endif /* MACHINE_SUN4 */


#ifdef MACHINE_EAGLE
#endif /* MACHINE_EAGLE */


#ifdef MACHINE_CONDOR
#  ifndef LITTLE_END
#    define LITTLE_END
#  endif /* LITTLE_END */
#endif /* MACHINE_CONDOR */


#ifdef MACHINE_SP
#  ifdef DOUBLE_REAL
Using doubles may cause problems writing binary files on the sp
because there is a bug in the xlc optimizer.
#  endif
#  include <float.h>
#  undef NTOHL
#  undef HTONL
#  undef NTOHS
#  undef HTONS
#  define HAVE_LIMITS_H
#  define HAVE_UNISTD_H
#endif /* MACHINE_SP */


#ifdef MACHINE_HYDRA
#  include <ieeefp.h>
#  include "stdarg.h" /* It has a screwed up version installed */
#  include <nan.h>
#  define isNaNf isnanf
#  define isdNaN innand
#endif /* MACHINE_HYDRA */


#include "sysext.h"


/***************************** DEFAULT SETTINGS ******************************/
#ifndef _H_STDARG
#  include <stdarg.h> /* needed for va_start */
#endif

#ifndef REAL_MATH
#  define LOG(x)   ((real) log((double) (x)))
#  define EXP(x)   ((real) exp((double) (x)))
#  define SQRT(x)  ((real) sqrt((double) (x)))
#  define POW(x,y) ((real) pow((double) (x), (double) (y)))
#  define ABS(x)   ((real) fabs((double) (x)))
#  define CEIL(x)  ((int)  ceil((double) (x)))
#  define FLOOR(x) ((int)  floor((double) (x)))
#endif /* REAL_MATH */

#ifdef LITTLE_END
#  define NaNfbytes {{ 0, 0, 0xc0, 0x7f }}
#  define NaNdbytes {{ 0, 0, 0, 0, 0, 0, 0xf8, 0xff }}
#  define NTOHL(x) ntohl(x)
#  define HTONL(x) htonl(x)
#  define NTOHS(x) ntohs(x)
#  define HTONS(x) htons(x)
#else
#  define NTOHL(x) (x)
#  define HTONL(x) (x)
#  define NTOHS(x) (x)
#  define HTONS(x) (x)
#  define NaNfbytes {{ 0x7f, 0xc0, 0, 0 }}
#  define NaNdbytes {{ 0xff, 0xf8, 0, 0, 0, 0, 0, 0 }}
#endif /* LITTLE_END */

union nanfunion {unsigned char __c[4]; float __r;};
extern union nanfunion NaNfunion;
union nandunion {unsigned char __c[8]; double __r;};
extern union nandunion NaNdunion;

#define NaNf NaNfunion.__r
#define NaNd NaNdunion.__r
#ifndef isNaNf
#  define isNaNf isnan
#endif /* isNaNf */
#ifndef isNaNd
#  define isNaNd isnan
#endif /* isNaNd */

#define IS_NEGATIVE(x) ((((int *) &x)[0] & (1 << 31)) != 0)

#ifdef FLOAT_REAL
#  define NaN   NaNf
#  define isNaN isNaNf
#else
#  define NaN   NaNd
#  define isNaN isNaNd
#  ifdef LITTLE_END
#    undef  IS_NEGATIVE
#    define IS_NEGATIVE(x) ((((int *) &x)[1] & (1 << 31)) != 0)
#  endif /* LITTLE_END */
#endif /* FLOAT_REAL */

/************************** LIMITS AND OTHER STUFF ***************************/

/* System Commands */
#define BUNZIP2  "bzip2 -d"
#define BZIP2    "bzip2 -1"
#define UNZIP    "gzip -d"
#define ZIP      "gzip -1"
#define COMPRESS "compress"

/* Miscellaneous */
#define MAX_FILENAME 512
#define MAX_PIPES    64
#define NO_VALUE "-"

#endif /* SYSTEM_H */
