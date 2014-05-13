check_PROGRAMS = test_ad_read test_ad_copy

TESTS = test_ad_read test_ad_copy
INCLUDES = \
	-I$(top_srcdir)/include/sphinxbase \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-DTESTDATADIR=\"$(top_srcdir)/test/regression\"

noinst_HEADERS = test_macros.h

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la \
	${top_builddir}/src/libsphinxad/libsphinxad.la
