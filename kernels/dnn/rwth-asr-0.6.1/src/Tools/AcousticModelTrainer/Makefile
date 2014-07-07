#!make

TOPDIR		= ../../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS		=
TARGETS		= acoustic-model-trainer$(exe) allophone-tool$(exe)

ACOUSTIC_MODEL_TRAINER_O = $(OBJDIR)/AcousticModelTrainer.o \
		  ../../Speech/libSprintSpeech.$(a) \
		  ../../Lattice/libSprintLattice.$(a) \
		  ../../Am/libSprintAm.$(a) \
		  ../../Mm/libSprintMm.$(a) \
		  ../../Lm/libSprintLm.$(a) \
		  ../../Mc/libSprintMc.$(a) \
		  ../../Legacy/libSprintLegacy.$(a) \
		  ../../Bliss/libSprintBliss.$(a) \
		  ../../Audio/libSprintAudio.$(a) \
		  ../../Signal/libSprintSignal.$(a) \
		  ../../Flow/libSprintFlow.$(a) \
		  ../../Math/libSprintMath.$(a) \
		  ../../Math/Lapack/libSprintMathLapack.$(a) \
		  ../../Core/libSprintCore.$(a) \
		  ../../Fsa/libSprintFsa.$(a)

ACOUSTIC_MODEL_TRAINER_O += $(subst src,../..,$(LIBS_SEARCH))

ALLOPHONE_TOOL_O= $(OBJDIR)/AllophoneTool.o \
		  ../../Am/libSprintAm.$(a) \
		  ../../Mm/libSprintMm.$(a) \
		  ../../Legacy/libSprintLegacy.$(a) \
		  ../../Bliss/libSprintBliss.$(a) \
		  ../../Fsa/libSprintFsa.$(a) \
		  ../../Mc/libSprintMc.$(a)

ifndef MODULE_IMAGE_ADAPT
ALLOPHONE_TOOL_O += ../../Core/libSprintCore.$(a)
ALLOPHONE_TOOL_O += ../../Math/Lapack/libSprintMathLapack.$(a)
endif

ifdef MODULE_CART
ACOUSTIC_MODEL_TRAINER_O += ../../Cart/libSprintCart.$(a)
ALLOPHONE_TOOL_O         += ../../Cart/libSprintCart.$(a)
endif
ifdef MODULE_FLF_CORE
ACOUSTIC_MODEL_TRAINER_O += ../../Flf/FlfCore/libSprintFlfCore.$(a)
ALLOPHONE_TOOL_O         += ../../Flf/FlfCore/libSprintFlfCore.$(a)
endif
ifdef MODULE_MATH_NR
ACOUSTIC_MODEL_TRAINER_O += ../../Math/Nr/libSprintMathNr.$(a)
ALLOPHONE_TOOL_O         += ../../Math/Nr/libSprintMathNr.$(a)
endif
ifdef MODULE_NN
ACOUSTIC_MODEL_TRAINER_O += ../../Nn/libSprintNn.$(a)
endif

# -----------------------------------------------------------------------------

build: $(TARGETS)

install:
	$(INSTALL) $(TARGETS) $(INSTALL_TARGET)

acoustic-model-trainer$(exe): $(ACOUSTIC_MODEL_TRAINER_O)
	$(LD) $(LD_START_GROUP) $^ $(LD_END_GROUP) -o $@ $(LDFLAGS)
#	$(LD) $^ -o $@ $(LDFLAGS)

allophone-tool$(exe): $(ALLOPHONE_TOOL_O)
	$(LD) $(LD_START_GROUP) $^ $(LD_END_GROUP) -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(patsubst %.o,%.d,$(filter %.o,$(ACOUSTIC_MODEL_TRAINER_O)))
sinclude $(patsubst %.o,%.d,$(filter %.o,$(ALLOPHONE_TOOL_O)))
# DO NOT DELETE
