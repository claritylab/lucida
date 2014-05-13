check_PROGRAMS = test_log_int16 test_log_int8 test_log_shifted
TESTS = test_log_int16 test_log_int8 test_log_shifted

INCLUDES = \
	-I$(top_srcdir)/include/sphinxbase \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la -lm

noinst_HEADERS = test_macros.h

CLEANFILES = *.out tmp.*
