noinst_LTLIBRARIES = libsphinxfe.la

libsphinxfe_la_SOURCES =			\
	fe_interface.c				\
	fe_noise.c				\
	fe_prespch_buf.c                        \
	fe_sigproc.c				\
	fe_warp_affine.c			\
	fe_warp.c				\
	fe_warp_inverse_linear.c		\
	fe_warp_piecewise_linear.c		\
	fixlog.c                                \
	yin.c

noinst_HEADERS =				\
	fe_internal.h				\
	fe_noise.h				\
	fe_prespch_buf.h                        \
	fe_type.h				\
	fe_warp_affine.h			\
	fe_warp.h				\
	fe_warp_inverse_linear.h		\
	fe_warp_piecewise_linear.h

INCLUDES = -I$(top_srcdir)/include/sphinxbase \
	   -I$(top_srcdir)/include \
           -I$(top_builddir)/include 

LIBOBJS = @LIBOBJS@
