/* include/sphinx_config.h, defaults for Win32 */
/* sphinx_config.h: Externally visible configuration parameters for
 * SphinxBase.
 */

/* Use ALSA library for sound I/O */
/* #undef AD_BACKEND_ALSA */

/* Use IRIX interface for sound I/O */
/* #undef AD_BACKEND_IRIX */

/* No interface for sound I/O */
/* #undef AD_BACKEND_NONE */

/* Use OSF interface for sound I/O */
/* #undef AD_BACKEND_OSF */

/* Use OSS interface for sound I/O */
/* #define AD_BACKEND_OSS */

/* Use OSS interface for sound I/O */
/* #undef AD_BACKEND_OSS_BSD */

/* Use SunOS interface for sound I/O */
/* #undef AD_BACKEND_SUNOS */

/* Use WinMM interface for sound I/O */
#undef AD_BACKEND_WIN32

/* Default radix point for fixed-point */
/* #undef DEFAULT_RADIX */

/* Enable thread safety */
#define ENABLE_THREADS 

/* The Thread Local Storage class */
#define SPHINXBASE_TLS __declspec(thread)

/* Use Q15 fixed-point computation */
/* #undef FIXED16 */

/* Use fixed-point computation */
/* #undef FIXED_POINT */

/* Enable matrix algebra with LAPACK */
#define WITH_LAPACK

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* Yes, we have errno.h */
#define HAVE_ERRNO_H 1

/* We don't have popen, but we do have _popen */
/* #define HAVE_POPEN 1 */

/* We do have perror */
#define HAVE_PERROR 1

/* We have sys/stat.h */
#define HAVE_SYS_STAT_H 1

/* We do not have unistd.h. */
#define YY_NO_UNISTD_H 1

/* Extension for executables */
#define EXEEXT ".exe"
