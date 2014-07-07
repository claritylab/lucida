# Intel(R) C / C++ Compiler Settings

# source $(ICCDIR)/bin/iccvars.sh

ICCDIR          = /usr/local/intel/cc/10.1
BINDIR          = $(ICCDIR)/bin
# BINDIR        = /rwthfs/rz/SW/UTIL/INTEL/compiler91-07.05.2007/cce/bin
GCC_VERSION	=

# -----------------------------------------------------------------------------
# Compiler
CC              = $(BINDIR)/icc
LD              = $(BINDIR)/icpc
CXX             = $(BINDIR)/icpc
CXXLD           = $(BINDIR)/icpc

CPPFLAGS        += $(SYS_INCLUDES)

GCC		= /usr/bin/gcc-4.4
# GCC		= /rwthfs/rz/SW/UTIL.common/gcc/3.4/x86_64-unknown-linux-gnu/bin/gcc

CCFLAGS	= -gcc-name=$(GCC) -cxxlib		# common for C and C++
# CCFLAGS		= -cxxlib		                # common for C and C++
CXXFLAGS        = $(CCFLAGS)    			# options for C++ compiler
CFLAGS		= $(CCFLAGS)				# options for C compiler


# -----------------------------------------------------------------------------
# compiler options
DEFINES		+= -D_GNU_SOURCE
CCFLAGS		+= -fno-exceptions
CCFLAGS		+= -funsigned-char
CXXFLAGS	+= -fpermissive
CFLAGS		+= -std=c99
CXXFLAGS	+= -std=gnu++98
CXXFLAGS        +=  -O3 -msse2
# CXXFLAGS        += -xT           # Optimize for Intel Core 2 Duo
# CCFLAGS	+= -Wall
LDFLAGS         += -L$(ICCDIR)/lib -lsvml
ifdef MODULE_OPENMP
CCFLAGS		+= -openmp
endif

ifeq ($(COMPILE),debug)
CCFLAGS		+= -g
DEFINES		+= -D_GLIBCXX_DEBUG
endif

ifneq ($(COMPILE),release)
# needed to get symbolic function names in stack traces (see Core/Assertions.cc)
LDFLAGS          += -rdynamic
endif

ifeq ($(PROFILE),bprof)
CCFLAGS		+= -g
LDFLAGS		+= /usr/lib/bmon.o
PROF		= bprof
endif
ifeq ($(PROFILE),gprof)
CCFLAGS		+= -pg
LDFLAGS		+= -pg  -static
PROF		= gprof -b
endif
ifeq ($(PROFILE),valgrind)
CCFLAGS		+= -g
LDFLAGS		+=
PROF		= echo "type: valgrind <execuable>"
endif
ifeq ($(PROFILE),purify)
PROFILE		+= "(not supported)"
PROF		=
endif
