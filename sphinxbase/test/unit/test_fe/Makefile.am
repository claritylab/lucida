check_PROGRAMS = test_fe test_pitch

TESTS = test_fe test_pitch
INCLUDES = \
	-I$(top_srcdir)/include/sphinxbase \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-DTESTDATADIR=\"$(top_srcdir)/test/regression\"

noinst_HEADERS = test_macros.h

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la
