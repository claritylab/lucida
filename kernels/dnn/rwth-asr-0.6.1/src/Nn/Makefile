#!gmake

TOPDIR		= ../..

include $(TOPDIR)/Makefile.cfg

CCFLAGS		+= -fexceptions

# -----------------------------------------------------------------------------

SUBDIRS 	=
TARGETS 	= libSprintNn.$(a) check$(exe)

LIBSPRINTNN_O	=						\
		$(OBJDIR)/Module.o				\


INCLUDES	+=

CHECK_O 	=						\
		  $(OBJDIR)/check.o				\
		  libSprintNn.$(a)				\
		  ../Core/libSprintCore.$(a)			\
		  ../Math/libSprintMath.$(a)			\
		  ../Math/Lapack/libSprintMathLapack.$(a)	\
		  ../Flow/libSprintFlow.$(a)			\
		  ../Signal/libSprintSignal.$(a)		\



ifdef MODULE_NN
  LIBSPRINTNN_O += $(OBJDIR)/ActivationLayer.o
  LIBSPRINTNN_O += $(OBJDIR)/Activations.o
  LIBSPRINTNN_O += $(OBJDIR)/BatchEstimator.o
  LIBSPRINTNN_O += $(OBJDIR)/BatchFeatureScorer.o
  LIBSPRINTNN_O += $(OBJDIR)/BufferedAlignedFeatureProcessor.o
  LIBSPRINTNN_O += $(OBJDIR)/BufferedFeatureExtractor.o
  LIBSPRINTNN_O += $(OBJDIR)/ClassLabelWrapper.o
  LIBSPRINTNN_O += $(OBJDIR)/Estimator.o
  LIBSPRINTNN_O += $(OBJDIR)/FeatureScorer.o
  LIBSPRINTNN_O += $(OBJDIR)/FeedForwardTrainer.o
  LIBSPRINTNN_O += $(OBJDIR)/LinearAndActivationLayer.o
  LIBSPRINTNN_O += $(OBJDIR)/LinearLayer.o
  LIBSPRINTNN_O += $(OBJDIR)/MeanNormalizedSgdEstimator.o
  LIBSPRINTNN_O += $(OBJDIR)/NetworkTopology.o
  LIBSPRINTNN_O += $(OBJDIR)/NeuralNetworkForwardNode.o
  LIBSPRINTNN_O += $(OBJDIR)/NeuralNetworkLayer.o
  LIBSPRINTNN_O += $(OBJDIR)/NeuralNetwork.o
  LIBSPRINTNN_O += $(OBJDIR)/NeuralNetworkTrainer.o
  LIBSPRINTNN_O += $(OBJDIR)/PreprocessingLayer.o
  LIBSPRINTNN_O += $(OBJDIR)/Prior.o
  LIBSPRINTNN_O += $(OBJDIR)/Regularizer.o
  LIBSPRINTNN_O += $(OBJDIR)/RpropEstimator.o
  LIBSPRINTNN_O += $(OBJDIR)/Statistics.o
endif






# -----------------------------------------------------------------------------
all: $(TARGETS)

.PHONY:	$(SUBDIRS)

interpol:
	$(MAKE) -C $@

libSprintNn.$(a): $(SUBDIRS) $(LIBSPRINTNN_O)
	$(MAKELIB) $@ $(LIBSPRINTNN_O) $(patsubst %,%/$(OBJDIR)/*.o,$(SUBDIRS))

check$(exe): $(CHECK_O)
	$(LD) $(LD_START_GROUP) $(CHECK_O) $(LD_END_GROUP) -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(LIBSPRINTNN_O:.o=.d)
sinclude $(patsubst %.o,%.d,$(filter %.o,$(CHECK_O)))
