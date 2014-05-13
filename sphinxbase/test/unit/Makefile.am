if USE_LAPACK
maybe_lapack = test_matrix
endif

if USE_THREADS
maybe_threads = test_thread
endif

SUBDIRS=test_ad					\
	test_alloc				\
	test_bitvec				\
	test_case				\
	test_string				\
	test_cmdln				\
	test_hash				\
	$(maybe_lapack)				\
	test_fe					\
	test_feat				\
	test_lineiter				\
	test_logmath				\
	test_ngram				\
	test_fsg				\
	test_util				\
	$(maybe_threads)

