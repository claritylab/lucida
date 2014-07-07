#!gmake

TOPDIR		= ../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS 	= FlfCore


ifdef MODULE_FLF
TARGETS		= libSprintFlf.$(a) check$(exe)
endif


LIBSPRINTFLF_O 	= \
		$(OBJDIR)/Archive.o \
		$(OBJDIR)/Best.o \
		$(OBJDIR)/Cache.o \
		$(OBJDIR)/CenterFrameConfusionNetworkBuilder.o \
		$(OBJDIR)/Combination.o \
		$(OBJDIR)/Compose.o \
		$(OBJDIR)/Concatenate.o \
		$(OBJDIR)/ConfusionNetwork.o \
		$(OBJDIR)/ConfusionNetworkCombination.o \
		$(OBJDIR)/ConfusionNetworkIo.o \
		$(OBJDIR)/CorpusProcessor.o \
		$(OBJDIR)/Copy.o \
		$(OBJDIR)/Convert.o \
		$(OBJDIR)/Draw.o \
		$(OBJDIR)/Determinize.o \
		$(OBJDIR)/EpsilonRemoval.o \
		$(OBJDIR)/Evaluate.o \
		$(OBJDIR)/Filter.o \
		$(OBJDIR)/Formattings.o \
		$(OBJDIR)/FlfIo.o \
		$(OBJDIR)/FwdBwd.o \
		$(OBJDIR)/GammaCorrection.o \
		$(OBJDIR)/HtkSlfIo.o \
		$(OBJDIR)/Info.o \
		$(OBJDIR)/Io.o \
		$(OBJDIR)/LanguageModel.o \
		$(OBJDIR)/LatticeAdaptor.o \
		$(OBJDIR)/LatticeHandler.o \
		$(OBJDIR)/Lexicon.o \
		$(OBJDIR)/LocalCostDecoder.o \
		$(OBJDIR)/Map.o \
		$(OBJDIR)/Miscellaneous.o \
		$(OBJDIR)/Module.o \
		$(OBJDIR)/NBest.o \
		$(OBJDIR)/Network.o \
		$(OBJDIR)/NodeFactory.o \
		$(OBJDIR)/NonWordFilter.o \
		$(OBJDIR)/PivotArcConfusionNetworkBuilder.o \
		$(OBJDIR)/Processor.o \
		$(OBJDIR)/Prune.o \
		$(OBJDIR)/Recognizer.o \
		$(OBJDIR)/Rescore.o \
        $(OBJDIR)/RescoreLm.o \
		$(OBJDIR)/RescoreInternal.o \
		$(OBJDIR)/Rescale.o \
		$(OBJDIR)/Segment.o \
		$(OBJDIR)/SegmentwiseSpeechProcessor.o \
		$(OBJDIR)/StateClusterConfusionNetworkBuilder.o \
		$(OBJDIR)/TimeAlignment.o \
		$(OBJDIR)/TimeframeConfusionNetwork.o \
		$(OBJDIR)/TimeframeConfusionNetworkBuilder.o \
		$(OBJDIR)/TimeframeConfusionNetworkCombination.o \
		$(OBJDIR)/TimeframeConfusionNetworkIo.o \
		$(OBJDIR)/TimeframeError.o \
		$(OBJDIR)/Traceback.o \
		$(OBJDIR)/Union.o

CHECK_O		= $(OBJDIR)/check.o \
		  FlfCore/libSprintFlfCore.$(a)
CHECK_O 	+= libSprintFlf.$(a) \
		  ../Speech/libSprintSpeech.$(a) \
		  ../Lattice/libSprintLattice.$(a) \
		  ../Lm/libSprintLm.$(a) \
		  ../Am/libSprintAm.$(a) \
		  ../Mm/libSprintMm.$(a) \
		  ../Mc/libSprintMc.$(a) \
		  ../Cart/libSprintCart.$(a) \
		  ../Bliss/libSprintBliss.$(a) \
		  ../Flow/libSprintFlow.$(a) \
		  ../Fsa/libSprintFsa.$(a) \
		  ../Legacy/libSprintLegacy.$(a) \
		  ../Core/libSprintCore.$(a)

# -----------------------------------------------------------------------------

all: $(TARGETS)

.PHONY: $(SUBDIRS)

FlfCore:
	$(MAKE) -C FlfCore libSprintFlfCore.$(a)

FlfExt:
	$(MAKE) -C FlfExt libSprintFlfExt.$(a)

libSprintFlf.$(a): $(SUBDIRS) $(LIBSPRINTFLF_O)
	$(MAKELIB) $@ $(LIBSPRINTFLF_O)

check$(exe): $(CHECK_O)
	$(LD) $(LD_START_GROUP) $^ $(LD_END_GROUP) -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(LIBSPRINTFLF_O:.o=.d)
sinclude $(patsubst %.o,%.d,$(filter %.o,$(CHECK_O)))
