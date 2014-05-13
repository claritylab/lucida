check_PROGRAMS = test_lineiter
TESTS = test_lineiter

INCLUDES = \
	-I$(top_srcdir)/include/sphinxbase \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-DFILEDIR=\"$(srcdir)\"

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la -lm

noinst_HEADERS = test_macros.h

EXTRA_DIST = test.txt

CLEANFILES = *.out tmp.*
