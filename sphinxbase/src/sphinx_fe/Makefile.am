bin_PROGRAMS = sphinx_fe

sphinx_fe_SOURCES = sphinx_fe.c

noinst_HEADERS = cmd_ln_defn.h			\
	sphinx_wave2feat.h

sphinx_fe_LDADD = \
	$(top_builddir)/src/libsphinxbase/libsphinxbase.la \
	@SNDFILE_LIBS@

INCLUDES = -I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/src/libsphinxbase/fe \
	@SNDFILE_CFLAGS@
