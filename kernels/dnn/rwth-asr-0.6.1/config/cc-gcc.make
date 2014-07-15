# GCC Compiler Settings

BINDIR          =
GCC_VERSION	=

# -----------------------------------------------------------------------------
# Compiler
CC		= $(BINDIR)gcc$(GCC_VERSION)
LD              = $(BINDIR)g++$(GCC_VERSION)
CXX             = $(BINDIR)g++$(GCC_VERSION)
CXXLD           = $(BINDIR)g++$(GCC_VERSION)

CPPFLAGS        += $(SYS_INCLUDES)

CCFLAGS		= 		# common for C and C++
CXXFLAGS        = $(CCFLAGS)    # options for C++ compiler
CFLAGS		= $(CCFLAGS)	# options for C compiler

CXX_MAJOR = $(shell $(CXX) --version | head -n 1 | sed -e 's/.*[ \t]\([0-9]\)\.\([0-9]\)\.\([0-9]\)\([ \t].*\)*$$/\1/')
CXX_MINOR = $(shell $(CXX) --version | head -n 1 | sed -e 's/.*[ \t]\([0-9]\)\.\([0-9]\)\.\([0-9]\)\([ \t].*\)*$$/\2/')

# -----------------------------------------------------------------------------
# compiler options
DEFINES		+= -D_GNU_SOURCE
CCFLAGS		+= -pipe
CCFLAGS		+= -funsigned-char
CCFLAGS		+= -fno-exceptions
CXXFLAGS	+= -fpermissive
CFLAGS		+= -std=c99
CXXFLAGS	+= -std=gnu++98
#CCFLAGS	+= -pedantic
CCFLAGS		+= -Wall
CCFLAGS		+= -Wno-long-long
#CXXFLAGS	+= -Woverloaded-virtual
#CFLAGS     += -Weffc++
#CFLAGS		+= -Wold-style-cast
#CCFLAGS         += -pg
#LDFLAGS         += -pg
ifdef MODULE_OPENMP
CCFLAGS		+= -fopenmp
LDFLAGS     += -fopenmp
endif

ifeq ($(strip $(CXX_MAJOR)),4)
ifeq ($(shell test $(CXX_MINOR) -ge 3 && echo 1),1)
# gcc >= 4.3

# code uses ext/hash_map, ext/hash_set etc.
CXXFLAGS += -Wno-deprecated

# strict type based alias analysis doesn't work with our implementation of
# reference counting smart pointers (Core::Ref)
CXXFLAGS += -fno-strict-aliasing
endif
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
