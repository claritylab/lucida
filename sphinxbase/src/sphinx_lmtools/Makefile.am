bin_PROGRAMS = sphinx_lm_eval sphinx_lm_convert

bin_SCRIPTS = sphinx_lm_sort
EXTRA_DIST = sphinx_lm_sort

LDADD = \
	$(top_builddir)/src/libsphinxbase/libsphinxbase.la \
	$(top_builddir)/src/libsphinxbase/lm/libsphinxlm.la

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include \
	-I$(top_srcdir)/src/libsphinxlm

