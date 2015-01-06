# $Id:$

lib_LTLIBRARIES = libcrfsuite.la

EXTRA_DIST = \
	crf.vcxproj 

libcrfsuite_la_SOURCES = \
	src/dictionary.c \
	src/logging.c \
	src/logging.h \
	src/params.c \
	src/params.h \
	src/quark.c \
	src/quark.h \
	src/rumavl.c \
	src/rumavl.h \
	src/vecmath.h \
	src/crfsuite_internal.h \
	src/dataset.c \
	src/holdout.c \
	src/train_arow.c \
	src/train_averaged_perceptron.c \
	src/train_l2sgd.c \
	src/train_lbfgs.c \
	src/train_passive_aggressive.c \
	src/crf1d.h \
	src/crf1d_context.c \
	src/crf1d_model.c \
	src/crf1d_feature.c \
	src/crf1d_encode.c \
	src/crf1d_tag.c \
	src/crfsuite_train.c \
	src/crfsuite.c

libcrfsuite_la_CFLAGS = -I$(top_builddir)/lib/cqdb/include

libcrfsuite_la_LDFLAGS = \
	-no-undefined \
	-release @VERSION@

libcrfsuite_la_LIBADD = \
	$(top_builddir)/lib/cqdb/libcqdb.la

AM_CFLAGS = @CFLAGS@
INCLUDES = @INCLUDES@
