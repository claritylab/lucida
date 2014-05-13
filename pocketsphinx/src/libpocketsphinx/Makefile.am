ARCH   = `uname -m | sed s/i.86/i386/`_`uname -s | tr A-Z a-z`

lib_LTLIBRARIES = libpocketsphinx.la

libpocketsphinx_la_LDFLAGS = -version-info 2:0:1 -lm

libpocketsphinx_la_SOURCES =			\
	acmod.c					\
	bin_mdef.c				\
	blkarray_list.c				\
	dict.c					\
	dict2pid.c				\
	fsg_history.c				\
	fsg_lextree.c				\
	fsg_search.c				\
	kws_search.c            \
	kws_detections.c        \
	hmm.c					\
	mdef.c					\
	ms_gauden.c				\
	ms_mgau.c				\
	ms_senone.c				\
	ngram_search.c				\
	ngram_search_fwdtree.c			\
	ngram_search_fwdflat.c			\
	phone_loop_search.c			\
	ps_alignment.c				\
	ps_lattice.c				\
	ps_mllr.c				\
	ptm_mgau.c				\
	s2_semi_mgau.c				\
	state_align_search.c			\
	tmat.c					\
	vector.c				\
	pocketsphinx.c

noinst_HEADERS =				\
	pocketsphinx_internal.h			\
	acmod.h					\
	ngram_search.h				\
	bin_mdef.h				\
	blkarray_list.h				\
	dict.h					\
	dict2pid.h				\
	fsg_history.h				\
	fsg_lextree.h				\
	fsg_search_internal.h			\
	kws_search.h            \
	kws_detections.h        \
	hmm.h					\
	mdef.h					\
	ms_gauden.h				\
	ms_mgau.h				\
	ms_senone.h				\
	ngram_search.h				\
	ngram_search_fwdtree.h			\
	ngram_search_fwdflat.h			\
	phone_loop_search.h			\
	ps_alignment.h				\
	ps_lattice_internal.h			\
	ptm_mgau.h				\
	s2_semi_mgau.h				\
	s3types.h				\
	state_align_search.h			\
	tied_mgau_common.h			\
	tmat.h					\
	vector.h

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-DMODELDIR=\"${prefix}/share/pocketsphinx/model\"

