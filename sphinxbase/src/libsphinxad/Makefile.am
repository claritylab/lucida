
# libsphinxad: Audio Device interface for Sphinx

lib_LTLIBRARIES = libsphinxad.la

# Confusing libtool version info means:
#
# This is API version 2, software release 1, which also supports API version 1
libsphinxad_la_LDFLAGS = -version-info 0:1:0

libsphinxad_la_SOURCES = cont_ad_base.c

EXTRA_libsphinxad_la_SOURCES = ad_base.c \
	ad_oss.c \
	ad_alsa.c \
	ad_pulse.c \
	ad_jack.c

EXTRA_DIST = ad_s60.cpp \
	play_win32.c \
	rec_win32.c

libsphinxad_la_DEPENDENCIES = @ad_files@

libsphinxad_la_LIBADD = \
	@ad_files@ \
	@ad_libs@ \
	@SAMPLERATE_LIBS@ \
	$(top_builddir)/src/libsphinxbase/libsphinxbase.la

INCLUDES = \
	@SAMPLERATE_CFLAGS@ \
	-I$(top_srcdir)/include/sphinxbase \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include
