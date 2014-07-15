#!gmake

CXXFLAGS       += -Wno-deprecated
LDFLAGS        += -liconv

MACOS_VERSION = $(shell sw_vers | awk '/ProductVersion/ { print $$2 }')
MACOS_MAJOR = $(shell echo $(MACOS_VERSION) | cut -f 1 -d .)
MACOS_MINOR = $(shell echo $(MACOS_VERSION) | cut -f 2 -d .)
ifeq ($(MACOS_MAJOR),10)
    ifeq ($(shell echo $(MACOS_MINOR) | awk '($$1 >= 5) { print 1 }'),1)
    	# MacOS X >= 10.5 is a 64-bit OS
    	PROC=x86_64
	CPU=i686
    endif
endif
# CXXFLAGS += -DMACOS_MAJOR=$(MACOS_MAJOR) -DMACOS_MINOR=$(MACOS_MINOR)


# -----------------------------------------------------------------------------
# GNU bison

BISON           = bison
BISON_MAJOR = $(shell $(BISON) --version | grep bison | sed -e 's/.* \([0-9]\{1,\}\)\.\([0-9]\{1,\}\).*/\1/')
BISON_MINOR = $(shell $(BISON) --version | grep bison | sed -e 's/.* \([0-9]\{1,\}\)\.\([0-9]\{1,\}\).*/\2/')
CXXFLAGS        += -DBISON_VERSION_$(BISON_MAJOR)
# -----------------------------------------------------------------------------
# special libraries
#
# If the libraries are not installed in a standard path, you need to setup the
# PKG_CONFIG_PATH variable. For example (assuming you are using a sh-based
# shell, like bash or zsh, and have installed/configured using './configure --prefix=$HOME')
#
#   PKG_CONFIG_PATH=${HOME}/lib/pkgconfig:${PKG_CONFIG_PATH}
#   export PKG_CONFIG_PATH
#

# FFmpeg-SVN

# OpenCV



# -----------------------------------------------------------------------------
# system Libraries

# PThreads
LDFLAGS         += -lpthread

# Common C++
#INCLUDES        += -I$(shell ccgnu2-config --includes)
#LDFLAGS         +=   $(shell ccgnu2-config --stdlibs)
#DEFINES		+= -DCCGNU_VERSION=$(shell ccgnu2-config --version | cut --delimiter '.' --fields 2)

# libXML
INCLUDES	+= $(shell xml2-config --cflags)
LDFLAGS		+= $(shell xml2-config --libs)

# zlib
LDFLAGS		+= -lz

# Lapack
LDFLAGS		+= -llapack -lblas
ifeq ($(PROFILE),gprof)
# This works around a problem with the current installation of Lapack at i6
LDFLAGS		+= -Xlinker --allow-multiple-definition
endif

# Free Lossless Audio Codec
ifdef MODULE_AUDIO_WAV_SYSTEM
CCFLAGS         += -I/opt/local/include
LDFLAGS         += -L/opt/local/lib -lsndfile
endif

# C library
ifeq ($(PROFILE),gprof)
LDFLAGS		+= -lm_p
LDFLAGS		+= -ldl_p -lpthread_p
LDFLAGS		+= -lc_p
else
LDFLAGS		+= -lm
endif

# FFT

# X11 and QT
X11_INCLUDE	= -I/usr/X11R6/include
X11_LIB		= -L/usr/X11R6/lib
QT_INCLUDE	= -I/usr/include/qt3
QT_LIB		= -L/usr/lib -lqt
MOC             = moc
XLDFLAGS        = $(X11_LIB) -lXpm -lXext -lX11

# -----------------------------------------------------------------------------
MAKE            = make
MAKEDEPEND	= makedepend -v -D__GNUC__=3 -D__GNUC_MINOR__=3
# AR usage: either 'ar rucs' or 'rm -f $@; ar qcs'
AR              = ar
ARFLAGS         = rucs
#MAKELIB	 = rm -f $@; $(AR) $(ARFLAGS)
MAKELIB		= $(AR) $(ARFLAGS)
ECHO            = @echo

