#!gmake

TOPDIR		= ../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS 	= Lapack
TARGETS		= libSprintMath.$(a) check$(exe)

LIBMATH_O	= \
		$(OBJDIR)/AcousticalAnalyticFunctions.o \
		$(OBJDIR)/AnalyticFunctionFactory.o \
		$(OBJDIR)/EigenvalueProblem.o \
		$(OBJDIR)/FastFourierTransform.o \
		$(OBJDIR)/Module.o \
		$(OBJDIR)/PiecewiseLinearFunction.o \
		$(OBJDIR)/CudaDataStructure.o

ifdef MODULE_MATH_NR
LIBMATH_O += $(OBJDIR)/Random.o
SUBDIRS   += Nr
endif

ifdef MODULE_SIGNAL_PLP
LIBMATH_O += $(OBJDIR)/LevinsonLse.o
endif

ifdef MODULE_CUDA
LIBMATH_O += $(OBJDIR)/CudaMatrixKernels.o
endif

CHECK_O		= $(OBJDIR)/check.o \
		libSprintMath.$(a) \
		Nr/libSprintMathNr.$(a) \
		Lapack/libSprintMathLapack.$(a) \
	        ../Core/libSprintCore.$(a)

# -----------------------------------------------------------------------------

default: build

build: libSprintMath.$(a) $(SUBDIRS)

all: $(TARGETS)

.PHONY: $(SUBDIRS)

Nr:
	$(MAKE) -C $@ libSprintMathNr.$(a)
Lapack:
	$(MAKE) -C $@ libSprintMathLapack.$(a)

ifdef MODULE_CUDA
$(OBJDIR)/CudaMatrixKernels.o: CudaMatrixKernels.cu
	$(NVCC) $(NVCCFLAGS) -o $(OBJDIR)/CudaMatrixKernels.o -c CudaMatrixKernels.cu
endif

libSprintMath.$(a): $(LIBMATH_O)
	$(MAKELIB) $@ $^

check$(exe): $(CHECK_O)
	$(LD) $^ -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(LIBMATH_O:.o=.d)
sinclude $(patsubst %.o,%.d,$(filter %.o,$(CHECK_O)))
