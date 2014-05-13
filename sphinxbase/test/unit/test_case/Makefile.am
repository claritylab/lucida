check_PROGRAMS = chgCase


INCLUDES = \
	-I$(top_srcdir)/include/sphinxbase \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la -lm

TESTS = _lcase1.test				\
	_lcase2.test				\
	_lcase3.test				\
	_strcmp1.test				\
	_strcmp2.test				\
	_strcmp3.test				\
	_ucase1.test				\
	_ucase2.test				\
	_ucase3.test

EXTRA_DIST = $(TESTS)
