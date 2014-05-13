check_PROGRAMS = \
	test_lm_read \
	test_lm_mmap \
	test_lm_score \
	test_lm_add \
	test_lm_recode \
	test_lm_casefold \
	test_lm_class \
	test_lm_set \
	test_lm_iter \
	test_lm_write

TESTS = $(check_PROGRAMS)

INCLUDES = \
	-I$(top_srcdir)/include/sphinxbase \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-DLMDIR=\"${srcdir}\"

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la

noinst_HEADERS = test_macros.h

EXTRA_DIST = 100.arpa.gz \
	100.arpa.bz2 \
	100.arpa.DMP \
	100.probdef \
	100.lmctl \
	100_2.arpa.gz \
	100_2.arpa.DMP \
	turtle.lm \
	turtle.lm.DMP \
	turtle.ug.lm \
	turtle.ug.lm.DMP

CLEANFILES = 100.tmp.arpa 100.tmp.DMP
