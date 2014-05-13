check_PROGRAMS = \
	test_ps_init \
	test_ps_reinit \
	test_ps_fwdtree \
	test_ps_fwdtree_fwdflat \
	test_ps_fwdflat \
	test_ps_fwdflat_bestpath \
	test_ps_fwdtree_bestpath \
	test_ps_simple \
	test_ps_nbest \
	test_ps_lattice \
	test_ps_set_search \
	test_acmod \
	test_acmod_grow \
	test_fwdtree \
	test_fwdflat \
	test_fwdtree_fwdflat \
	test_fwdtree_bestpath \
	test_fwdtree_nbest \
	test_pl_fwdtree \
	test_posterior \
	test_fsg \
	test_fsg2 \
	test_fsg3 \
	test_jsgf \
	test_lm_read \
	test_dict \
	test_dict2pid \
	test_senfh \
	test_alignment \
	test_state_align \
	test_mllr

TESTS = $(check_PROGRAMS)

EXTRA_DIST = ps_test.c

noinst_HEADERS = test_macros.h

INCLUDES = -I$(top_srcdir)/include \
	-I$(top_srcdir)/src/libpocketsphinx \
	-I$(top_builddir)/include \
	-I$(srcdir) \
	-DMODELDIR=\"${top_srcdir}/model\" \
	-DDATADIR=\"${top_srcdir}/test/data\"

LDADD = \
	$(top_builddir)/src/libpocketsphinx/libpocketsphinx.la \
	-lsphinxbase

CLEANFILES = *.log *.out *.lat *.mfc *.raw *.dic *.sen

valgrind-check:
	for testf in .libs/lt-*; do valgrind --leak-check=full --show-reachable=yes \
	    $$testf; done >& valgrind.log

