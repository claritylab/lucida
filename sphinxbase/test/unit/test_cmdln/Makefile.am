check_PROGRAMS = cmdln_parse cmdln_parse_multiple cmdln_parse_r

INCLUDES = \
	-I$(top_srcdir)/include/sphinxbase \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la -lm


TESTS = _test_parse_badargs.test		\
	_test_parse_defaults_r.test		\
	_test_parse_defaults.test		\
	_test_parse_goodargs.test		\
	_test_parse_multiple.test


EXTRA_DIST = $(TESTS)				\
	_test_parse_badargs.res			\
	_test_parse_defaults.res		\
	_test_parse_defaults_r.res		\
	_test_parse_goodargs.res		\
	_test_parse_multiple.res
