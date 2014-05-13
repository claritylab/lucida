bin_PROGRAMS = sphinx_cont_adseg sphinx_cont_fileseg sphinx_pitch

sphinx_cont_adseg_SOURCES = cont_adseg.c
sphinx_cont_fileseg_SOURCES = cont_fileseg.c

LDADD = $(top_builddir)/src/libsphinxbase/libsphinxbase.la \
	 $(top_builddir)/src/libsphinxad/libsphinxad.la

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
