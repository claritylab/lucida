# Generate code for Pentium Pro and beyond.
# Change this to -mcpu=i386 if you want to support older CPUs.

ifeq ($(COMPILER),gcc)
CCFLAGS		+= -march=i686
CCFLAGS		+= -ffast-math
endif



# Uncomment the following lines to enable the
# SSE2 L2Norm code generator instead of the
# MMX code generator
# DEFINES         += -DENABLE_SSE2
# CCFLAGS         += -msse2

# Use -DDISABLE_SIMD to use the feature scorer
# without optimized l2norm computation.
# DEFINES         += -DDISABLE_SIMD

ifeq ($(COMPILE),standard)
CCFLAGS		+= -O2
endif
ifeq ($(COMPILE),release)
CCFLAGS		+= -O3
endif
ifeq ($(COMPILE),debug)
CCFLAGS		+= -O0
endif

ifeq ($(PROFILE),valgrind)
DEFINES		+= -DDISABLE_SIMD
endif

