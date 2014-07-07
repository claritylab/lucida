#!gmake

TOPDIR		= ../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

TARGETS		= libSprintMm.$(a) check$(exe)

LIBSPRINTMM_O	= \
		$(OBJDIR)/AbstractMixtureSetEstimator.o \
		$(OBJDIR)/CombinedFeatureScorer.o \
		$(OBJDIR)/CovarianceFeatureScorerElement.o \
		$(OBJDIR)/CovarianceWeightedFeatureScorerElement.o \
		$(OBJDIR)/Feature.o \
		$(OBJDIR)/GaussDensity.o \
		$(OBJDIR)/GaussDensityEstimator.o \
		$(OBJDIR)/GaussDiagonalMaximumFeatureScorer.o \
		$(OBJDIR)/IntelCodeGenerator.o \
		$(OBJDIR)/IntelOptimization.o \
		$(OBJDIR)/Mixture.o \
		$(OBJDIR)/MixtureEstimator.o \
		$(OBJDIR)/MixtureFeatureScorerElement.o \
		$(OBJDIR)/MixtureSet.o \
		$(OBJDIR)/MixtureSetBuilder.o \
		$(OBJDIR)/MixtureSetEstimator.o \
		$(OBJDIR)/MixtureSetLoader.o \
		$(OBJDIR)/MixtureSetReader.o \
		$(OBJDIR)/MixtureSetSplitter.o \
		$(OBJDIR)/MixtureSetTopology.o \
		$(OBJDIR)/Module.o \
		$(OBJDIR)/SimdFeatureScorer.o \
		$(OBJDIR)/SSE2CodeGenerator.o \
		$(OBJDIR)/Utilities.o

CHECK_O		= $(OBJDIR)/check.o \
		  ../Legacy/libSprintLegacy.$(a) \
		  ../Math/libSprintMath.$(a) \
		  ../Math/Lapack/libSprintMathLapack.$(a) \
		  libSprintMm.$(a) \
		  ../Fsa/libSprintFsa.$(a) \
		  ../Core/libSprintCore.$(a) \
		  ../Mc/libSprintMc.$(a)


ifdef MODULE_MM_DT
LIBSPRINTMM_O += $(OBJDIR)/ConvertGaussDensityEstimator.o
LIBSPRINTMM_O += $(OBJDIR)/ConvertMixtureEstimator.o
LIBSPRINTMM_O += $(OBJDIR)/ConvertMixtureSetEstimator.o
LIBSPRINTMM_O += $(OBJDIR)/DiscriminativeGaussDensityEstimator.o
LIBSPRINTMM_O += $(OBJDIR)/DiscriminativeMixtureEstimator.o
LIBSPRINTMM_O += $(OBJDIR)/DiscriminativeMixtureSetEstimator.o
LIBSPRINTMM_O += $(OBJDIR)/EbwDiscriminativeGaussDensityEstimator.o
LIBSPRINTMM_O += $(OBJDIR)/EbwDiscriminativeMixtureEstimator.o
LIBSPRINTMM_O += $(OBJDIR)/EbwDiscriminativeMixtureSetEstimator.o
LIBSPRINTMM_O += $(OBJDIR)/ISmoothingGaussDensityEstimator.o
LIBSPRINTMM_O += $(OBJDIR)/ISmoothingMixtureEstimator.o
LIBSPRINTMM_O += $(OBJDIR)/ISmoothingMixtureSetEstimator.o
LIBSPRINTMM_O += $(OBJDIR)/IterationConstants.o
LIBSPRINTMM_O += $(OBJDIR)/RpropOptimization.o
LIBSPRINTMM_O += $(OBJDIR)/RpropDiscriminativeGaussDensityEstimator.o
LIBSPRINTMM_O += $(OBJDIR)/RpropDiscriminativeMixtureEstimator.o
LIBSPRINTMM_O += $(OBJDIR)/RpropDiscriminativeMixtureSetEstimator.o
#MODF DensityToWeightMap.hh
#MODF RpropOptimization.hh
#MODF RpropOptimization.tcc
endif

ifdef MODULE_MM_BATCH
LIBSPRINTMM_O += $(OBJDIR)/BatchFeatureScorer.o
$(OBJDIR)/BatchFeatureScorer.o : CXXFLAGS += -msse2
LIBSPRINTMM_O += $(OBJDIR)/DensityClustering.o
#MODF DensityClustering.tcc
endif





ifdef MODULE_ADAPT_MLLR
LIBSPRINTMM_O += $(OBJDIR)/MllrAdaptation.o
endif

ifdef MODULE_ADAPT_CMLLR
LIBSPRINTMM_O += $(OBJDIR)/AffineFeatureTransformAccumulator.o
endif




# -----------------------------------------------------------------------------

all: $(TARGETS)


.PHONY:	$(SUBDIRS)

libSprintMm.$(a): $(SUBDIRS) $(LIBSPRINTMM_O)
	$(MAKELIB) $@ $(LIBSPRINTMM_O) $(patsubst %,%/$(OBJDIR)/*.o,$(SUBDIRS))

check$(exe): $(CHECK_O)
	$(LD) $(LD_START_GROUP) $^ $(LD_END_GROUP) -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(LIBSPRINTMM_O:.o=.d)
sinclude $(patsubst %.o,%.d,$(filter %.o,$(CHECK_O)))
