#!gmake

LD_START_GROUP = -Wl,-\(
LD_END_GROUP   = -Wl,-\)

# -----------------------------------------------------------------------------
# GNU bison

BISON           = bison
BISON_MAJOR = $(shell $(BISON) --version | grep bison | sed -e 's/.* \([0-9]\+\)\.\([0-9]\+\).*/\1/')
BISON_MINOR = $(shell $(BISON) --version | grep bison | sed -e 's/.* \([0-9]\+\)\.\([0-9]\+\).*/\2/')
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
LDFLAGS     += -lpthread
# Required for profiling, eg. clock_gettime
LDFLAGS     += -lrt

# Common C++
#INCLUDES   += -I$(shell ccgnu2-config --includes)
#LDFLAGS    +=   $(shell ccgnu2-config --stdlibs)
#DEFINES    += -DCCGNU_VERSION=$(shell ccgnu2-config --version | cut --delimiter '.' --fields 2)

# libXML
INCLUDES    += $(shell xml2-config --cflags)
LDFLAGS     += $(shell xml2-config --libs)

# zlib
LDFLAGS     += -lz

# Lapack and Blas
ifeq (1,1)
ifeq (1,1)
LDFLAGS     += -lblas
LDFLAGS     += -llapack
endif
endif

ifdef MODULE_CUDA
CUDAROOT    = /usr/local/cuda
INCLUDES    += -I$(CUDAROOT)/include/
LDFLAGS     += -L$(CUDAROOT)/lib64/ -lcublas -lcudart -lcurand
NVCC        = $(CUDAROOT)/bin/nvcc
# optimal for GTX680; set sm_35 for K20
NVCCFLAGS   = -arch sm_30
endif

ifneq ($(shell ldd /usr/lib/liblapack.so | grep g2c),)
    LDFLAGS += -lg2c
endif
ifeq ($(PROFILE),gprof)
# This works around a problem with the current installation of Lapack at i6
LDFLAGS     += -Xlinker --allow-multiple-definition
endif

# Free Lossless Audio Codec
ifdef MODULE_AUDIO_WAV_SYSTEM
LDFLAGS     += -lsndfile
endif

# C library
ifeq ($(PROFILE),gprof)
LDFLAGS     += -lm_p
LDFLAGS     += -ldl_p -lpthread_p
LDFLAGS     += -lc_p
else
LDFLAGS     += -lm
endif


# X11 and QT
X11_INCLUDE = -I/usr/X11R6/include
X11_LIB     = -L/usr/X11R6/lib
QT_INCLUDE  = -I/usr/include/qt3
QT_LIB      = -L/usr/lib -lqt
MOC         = moc
XLDFLAGS    = $(X11_LIB) -lXpm -lXext -lX11

# -----------------------------------------------------------------------------
MAKE        = make
MAKEDEPEND  = makedepend -v -D__GNUC__=3 -D__GNUC_MINOR__=3
# AR usage: either 'ar rucs' or 'rm -f $@; ar qcs'
AR          = ar
ARFLAGS     = rucs
#MAKELIB     = rm -f $@; $(AR) $(ARFLAGS)
ifeq ($(COMPILER),sun)
MAKELIB     = $(CXX) $(CXXFLAGS) -xar -o
else ifeq ($(COMPILE),debug_dynamic)
MAKELIB     = $(LD) $(LDFLAGS) -shared -o
else
MAKELIB     = $(AR) $(ARFLAGS)
endif
ECHO        = @/bin/echo -e
