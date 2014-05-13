
check_PROGRAMS = test_solve test_invert test_determinant

test_solve_SRCS = test_solve.c
test_invert_SRCS = test_invert.c
test_determinant_SRCS = test_determinant.c

INCLUDES = \
	-I$(top_srcdir)/include/sphinxbase \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la -lm


TESTS = _test_determinant.test			\
	_test_invert.test			\
	_test_solve.test

EXTRA_DIST = _test_determinant.res		\
	_test_determinant.test			\
	_test_invert.res			\
	_test_invert.test			\
	_test_solve.res				\
	_test_solve.test

CLEANFILES = *.out
