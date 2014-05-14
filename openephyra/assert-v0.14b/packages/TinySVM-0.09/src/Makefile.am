# Process this file with automake to produce Makefile.in
#
# $Id: Makefile.am,v 1.19 2002/08/20 03:00:45 taku-ku Exp $;
#
AUTOMAKE_OPTIONS = no-dependencies


lib_LTLIBRARIES = libtinysvm.la
libtinysvm_la_SOURCES = getopt.c getopt1.c getopt.h common.h \
			param.cpp param.h \
			kernel.h \
			classifier.h classifier.cpp \
		        q_matrix.h q_matrix.cpp \
		        cache.h timer.h \
			base_example.cpp base_example.h \
			example.cpp example.h \
			model.cpp model.h \
			base_solver.h \
			oneclass_solver.cpp oneclass_solver.h \
			svm_solver.cpp svm_solver.h \
			svr_solver.cpp svr_solver.h \
			qp_solver.h qp_solver.cpp \
			tinysvm.h

libtinysvm_la_LDFLAGS = -no-undefined -version-info $(LTVERSION)
include_HEADERS = tinysvm.h
pkginclude_HEADERS = model.h example.h base_example.h param.h \
                     misc.h kernel.h classifier.h

bin_PROGRAMS = svm_learn svm_classify svm_model

svm_learn_SOURCES = svm_learn.cpp
svm_learn_LDADD = libtinysvm.la

svm_classify_SOURCES = svm_classify.cpp
svm_classify_LDADD = libtinysvm.la

svm_model_SOURCES = svm_model.cpp
svm_model_LDADD = libtinysvm.la

