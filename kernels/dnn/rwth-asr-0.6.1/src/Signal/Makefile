#!gmake

TOPDIR		= ../..

include $(TOPDIR)/Makefile.cfg

CFLAGS		+= `$(BINDIR)xml2-config --cflags`
LDFLAGS		+= `$(BINDIR)xml2-config --libs`

# -----------------------------------------------------------------------------

SUBDIRS 	=
TARGETS		= libSprintSignal.$(a) check$(exe)

LIBSPRINTSIGNAL_O	= \
			  $(OBJDIR)/CosineTransform.o \
			  $(OBJDIR)/DcDetection.o \
			  $(OBJDIR)/Delay.o \
			  $(OBJDIR)/Delimiter.o \
			  $(OBJDIR)/EigenTransform.o \
			  $(OBJDIR)/FramePrediction.o \
			  $(OBJDIR)/FastFourierTransform.o \
			  $(OBJDIR)/Filterbank.o \
			  $(OBJDIR)/Module.o \
			  $(OBJDIR)/Mrasta.o \
			  $(OBJDIR)/Normalization.o \
			  $(OBJDIR)/Preemphasis.o \
			  $(OBJDIR)/RepeatingFramePrediction.o \
			  $(OBJDIR)/Regression.o \
			  $(OBJDIR)/ScatterEstimator.o \
			  $(OBJDIR)/ScatterTransform.o \
			  $(OBJDIR)/SegmentClustering.o \
			  $(OBJDIR)/SegmentEstimator.o \
			  $(OBJDIR)/VectorSequenceAggregation.o \
			  $(OBJDIR)/VectorSequenceConcatenation.o \
			  $(OBJDIR)/VectorResize.o \
			  $(OBJDIR)/Window.o \
			  $(OBJDIR)/WindowBuffer.o \
			  $(OBJDIR)/WindowFunction.o

CHECK_O			= $(OBJDIR)/check.o \
			  libSprintSignal.$(a) \
			  ../Flow/libSprintFlow.$(a) \
			  ../Bliss/libSprintBliss.$(a) \
			  ../Mm/libSprintMm.$(a) \
			  ../Math/libSprintMath.$(a) \
			  ../Math/Lapack/libSprintMathLapack.$(a) \
			  ../Mc/libSprintMc.$(a) \
			  ../Core/libSprintCore.$(a) \
			  ../Legacy/libSprintLegacy.$(a)


ifdef MODULE_SIGNAL_GAMMATONE
LIBSPRINTSIGNAL_O += $(OBJDIR)/GammaTone.o
LIBSPRINTSIGNAL_O += $(OBJDIR)/SpectralIntegration.o
LIBSPRINTSIGNAL_O += $(OBJDIR)/TemporalIntegration.o
LIBSPRINTSIGNAL_O += $(OBJDIR)/TimeWindowBuffer.o
endif
ifdef MODULE_SIGNAL_VTLN
LIBSPRINTSIGNAL_O += $(OBJDIR)/LikelihoodFunction.o
LIBSPRINTSIGNAL_O += $(OBJDIR)/BayesClassification.o
LIBSPRINTSIGNAL_O += $(OBJDIR)/AprioriProbability.o
endif
ifdef MODULE_SIGNAL_VOICEDNESS
LIBSPRINTSIGNAL_O += $(OBJDIR)/CrossCorrelation.o
LIBSPRINTSIGNAL_O += $(OBJDIR)/PeakDetection.o
endif
ifdef MODULE_SIGNAL_PLP
LIBSPRINTSIGNAL_O += $(OBJDIR)/ArEstimator.o
LIBSPRINTSIGNAL_O += $(OBJDIR)/VectorTransform.o
LIBSPRINTSIGNAL_O += $(OBJDIR)/AutoregressionToCepstrum.o
LIBSPRINTSIGNAL_O += $(OBJDIR)/AutoregressionToSpectrum.o
endif


# -----------------------------------------------------------------------------

all: $(TARGETS)

libSprintSignal.$(a): $(LIBSPRINTSIGNAL_O)
	$(MAKELIB) $@ $^
check$(exe): $(CHECK_O)
	$(LD) $^ -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(LIBSPRINTSIGNAL_O:.o=.d)
sinclude $(patsubst %.o,%.d,$(filter %.o,$(CHECK_O)))
