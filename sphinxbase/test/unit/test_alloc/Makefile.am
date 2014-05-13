check_PROGRAMS = test_ckd_alloc test_ckd_alloc_catch test_ckd_alloc_fail test_ckd_alloc_abort \
	test_listelem_alloc

TESTS = test_ckd_alloc test_ckd_alloc_catch test_ckd_alloc_fail.sh test_ckd_alloc_abort.sh \
	test_listelem_alloc
INCLUDES = \
	-I$(top_srcdir)/include/sphinxbase \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-DTESTDATADIR=\"$(top_srcdir)/test/regression\"

noinst_HEADERS = test_macros.h
EXTRA_DIST = test_ckd_alloc_fail.sh test_ckd_alloc_abort.sh

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la
