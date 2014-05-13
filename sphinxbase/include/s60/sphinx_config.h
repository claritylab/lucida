/* include/sphinx_config.h, defaults for S60.  */
/* sphinx_config.h: Externally visible configuration parameters for
 * SphinxBase.
 */

/* Use S60 interface for sound I/O */
#define AD_BACKEND_S60

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* Define to 1 if the system has the type `long long'. */
#define HAVE_LONG_LONG 1

/* The size of `long long', as computed by sizeof. */
#define SIZEOF_LONG_LONG 8

/* Enable debugging output */
#undef SPHINX_DEBUG

/* Extension for executables */
#define EXEEXT ".exe"

/* Enable thread safety */
#define ENABLE_THREADS 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `m' library (-lm). */
#define HAVE_LIBM 1

/* Define to 1 if you have the `pthread' library (-lpthread). */
#define HAVE_LIBPTHREAD 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `perror' function. */
#define HAVE_PERROR 1

/* Define to 1 if you have the `popen' function. */
#define HAVE_POPEN 1

/* Define to 1 if you have the <pthread.h> header file. */
#define HAVE_PTHREAD_H 1

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Enable matrix algebra with LAPACK */
#define WITH_LAPACK 1
