# Process this file with automake to produce Makefile.in
#
# $Id: Makefile.am,v 1.12 2003/01/06 10:46:35 taku-ku Exp $;
#
AUTOMAKE_OPTIONS = no-dependencies 
lib_LTLIBRARIES = libyamcha.la
libyamcha_la_LDFLAGS = -no-undefined -version-info $(LTVERSION) -lm
libyamcha_la_SOURCES = param.h param.cpp \
                 svm.cpp common.h yamcha.h mmap.h \
		 feature_index.h feature_index.cpp \
		 chunker.cpp darts.h chunkersub.h
		 
include_HEADERS = yamcha.h

bin_PROGRAMS = yamcha
yamcha_SOURCES = yamcha.cpp
yamcha_LDADD = libyamcha.la
