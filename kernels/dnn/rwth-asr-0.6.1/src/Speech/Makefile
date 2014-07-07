#!gmake

TOPDIR		= ../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS		=
TARGETS		= libSprintSpeech.$(a) check$(exe)

LIBSPRINTSPEECH_O	= \
		$(OBJDIR)/AcousticModelTrainer.o 		\
		$(OBJDIR)/AlignerModelAcceptor.o 		\
		$(OBJDIR)/AligningFeatureExtractor.o 		\
		$(OBJDIR)/Alignment.o 				\
		$(OBJDIR)/AlignmentNode.o			\
		$(OBJDIR)/AlignmentWithLinearSegmentation.o 	\
		$(OBJDIR)/AllophoneStateGraphBuilder.o		\
		$(OBJDIR)/CorpusProcessor.o 			\
		$(OBJDIR)/CorpusVisitor.o 			\
		$(OBJDIR)/CovarianceEstimator.o			\
		$(OBJDIR)/DataExtractor.o 			\
		$(OBJDIR)/DataSource.o	 			\
		$(OBJDIR)/DelayedRecognizer.o       \
		$(OBJDIR)/Feature.o 				\
		$(OBJDIR)/FeatureScorer.o			\
		$(OBJDIR)/FsaCache.o				\
		$(OBJDIR)/LabelingFeatureExtractor.o		\
		$(OBJDIR)/MixtureSetTrainer.o 			\
		$(OBJDIR)/ModelCombination.o 			\
		$(OBJDIR)/Module.o 			        \
		$(OBJDIR)/Recognizer.o 				\
		$(OBJDIR)/ScatterMatricesEstimator.o		\
		$(OBJDIR)/TextDependentSequenceFiltering.o 	\
		$(OBJDIR)/TextIndependentMixtureSetTrainer.o

CHECK_O		= $(OBJDIR)/check.o \
		  libSprintSpeech.$(a) \
		  ../Lm/libSprintLm.$(a) \
		  ../Am/libSprintAm.$(a) \
		  ../Mm/libSprintMm.$(a) \
		  ../Mc/libSprintMc.$(a) \
		  ../Search/libSprintSearch.$(a) \
		  ../Legacy/libSprintLegacy.$(a) \
		  ../Bliss/libSprintBliss.$(a) \
		  ../Flow/libSprintFlow.$(a) \
		  ../Fsa/libSprintFsa.$(a) \
		  ../Core/libSprintCore.$(a) \
		  ../Lattice/libSprintLattice.$(a) \
		  ../Math/libSprintMath.$(a) \
		  ../Math/Lapack/libSprintMathLapack.$(a)

ifdef MODULE_SPEECH_LATTICE_ALIGNMENT
LIBSPRINTSPEECH_O += $(OBJDIR)/PhonemeSequenceAlignmentGenerator.o
LIBSPRINTSPEECH_O += $(OBJDIR)/SegmentwiseAlignmentGenerator.o
LIBSPRINTSPEECH_O += $(OBJDIR)/SegmentwiseFeatureExtractor.o
endif

ifdef MODULE_SPEECH_DT
LIBSPRINTSPEECH_O += $(OBJDIR)/AbstractSegmentwiseTrainer.o
LIBSPRINTSPEECH_O += $(OBJDIR)/AccuracyFsaBuilder.o
LIBSPRINTSPEECH_O += $(OBJDIR)/AcousticSegmentwiseTrainer.o
LIBSPRINTSPEECH_O += $(OBJDIR)/Confidences.o
LIBSPRINTSPEECH_O += $(OBJDIR)/DiscriminativeMixtureSetTrainer.o
LIBSPRINTSPEECH_O += $(OBJDIR)/EbwDiscriminativeMixtureSetTrainer.o
LIBSPRINTSPEECH_O += $(OBJDIR)/LatticeExtractor.o
LIBSPRINTSPEECH_O += $(OBJDIR)/LatticeSetExtractor.o
LIBSPRINTSPEECH_O += $(OBJDIR)/LatticeSetProcessor.o
LIBSPRINTSPEECH_O += $(OBJDIR)/PruningLatticeSetNode.o
LIBSPRINTSPEECH_O += $(OBJDIR)/SegmentwiseGmmTrainer.o
LIBSPRINTSPEECH_O += $(OBJDIR)/WordLatticeExtractor.o
#MODF LatticeExtractorAutomaton.hh
endif



ifdef MODULE_SPEECH_LATTICE_FLOW_NODES
LIBSPRINTSPEECH_O += $(OBJDIR)/AlignmentFromLattice.o
LIBSPRINTSPEECH_O += $(OBJDIR)/LatticeNodes.o
LIBSPRINTSPEECH_O += $(OBJDIR)/LatticeArcAccumulator.o
CHECK_O           += ../Flf/FlfCore/libSprintFlfCore.$(a)
endif

ifdef MODULE_SPEECH_ALIGNMENT_FLOW_NODES
LIBSPRINTSPEECH_O += $(OBJDIR)/AlignmentGeneratorNode.o
LIBSPRINTSPEECH_O += $(OBJDIR)/AlignmentTransformNode.o
LIBSPRINTSPEECH_O += $(OBJDIR)/SegmentNode.o
#MODF SegmentwiseFeatures.hh
endif


ifdef MODULE_ADAPT_MLLR
LIBSPRINTSPEECH_O += $(OBJDIR)/ModelTransformEstimator.o
LIBSPRINTSPEECH_O += $(OBJDIR)/FeatureShiftAdaptor.o
endif
ifdef MODULE_ADAPT_CMLLR
LIBSPRINTSPEECH_O += $(OBJDIR)/AffineFeatureTransformEstimator.o
LIBSPRINTSPEECH_O += $(OBJDIR)/KeyedEstimator.o
endif
ifdef MODULE_CART
LIBSPRINTSPEECH_O += $(OBJDIR)/DecisionTreeTrainer.o
CHECK_O		  += ../Cart/libSprintCart.$(a)
endif


ifdef MODULE_MATH_NR
CHECK_O		  += ../Math/Nr/libSprintMathNr.$(a)
endif


# -----------------------------------------------------------------------------

all: $(TARGETS)

libSprintSpeech.$(a): $(LIBSPRINTSPEECH_O)
	$(MAKELIB) $@ $^

check$(exe): $(CHECK_O)
	$(LD) -Wl,-\( $(CHECK_O) -Wl,-\) -o check$(exe) $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(LIBSPRINTSPEECH_O:.o=.d)
sinclude $(patsubst %.o,%.d,$(filter %.o,$(CHECK_O)))
