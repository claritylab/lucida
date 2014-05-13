check_PROGRAMS = \
	test_fsg_read \
	test_fsg_jsgf \
	test_fsg_write_fsm

TESTS = $(check_PROGRAMS)

INCLUDES = \
	-I$(top_srcdir)/include/sphinxbase \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-DLMDIR=\"${srcdir}\"

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la

noinst_HEADERS = test_macros.h

EXTRA_DIST = goforward.fsg polite.gram public.gram
