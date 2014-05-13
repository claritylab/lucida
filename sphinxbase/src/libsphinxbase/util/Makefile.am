noinst_LTLIBRARIES = libsphinxutil.la

if LAPACK_LITE
LAPACK_LITE_SRCS = \
	slamch.c \
	slapack_lite.c \
	blas_lite.c \
	f2c_lite.c
endif

libsphinxutil_la_SOURCES = \
	bitvec.c \
	bio.c \
	case.c \
	ckd_alloc.c \
	dtoa.c \
	listelem_alloc.c \
	cmd_ln.c \
	err.c \
	filename.c \
	genrand.c \
	glist.c \
	hash_table.c \
	heap.c \
	huff_code.c \
	logmath.c \
	mmio.c \
	pio.c \
	matrix.c \
	profile.c \
	sbthread.c \
	strfuncs.c \
	utf8.c \
	$(LAPACK_LITE_SRCS)

EXTRA_DIST = \
	slamch.c \
	slapack_lite.c \
	blas_lite.c \
	f2c_lite.c \
	string_wce.c \
	errno.c

INCLUDES = -I$(top_srcdir)/include/sphinxbase \
	   -I$(top_srcdir)/include \
           -I$(top_builddir)/include 

LIBOBJS = @LIBOBJS@
