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

/* Use Q15 fixed-point computation */
/* #undef FIXED16 */

/* Use fixed-point computation */
/* #undef FIXED_POINT */

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG 4
