#!make

TOPDIR		= ../../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS		=
TARGETS		= tagging-trainer$(exe)

TAGGING_TRAINER_O = $(OBJDIR)/TaggingTrainer.o \
		../../Core/libSprintCore.$(a) \
		../../Speech/libSprintSpeech.$(a) \
		../../Lattice/libSprintLattice.$(a) \
		../../Lm/libSprintLm.$(a) \
		../../Am/libSprintAm.$(a) \
		../../Mm/libSprintMm.$(a) \
		../../Mc/libSprintMc.$(a) \
		../../Legacy/libSprintLegacy.$(a) \
		../../Bliss/libSprintBliss.$(a) \
		../../Audio/libSprintAudio.$(a) \
		../../Signal/libSprintSignal.$(a) \
		../../Flow/libSprintFlow.$(a) \
		../../Math/libSprintMath.$(a) \
		../../Math/Lapack/libSprintMathLapack.$(a) \
		../../Fsa/libSprintFsa.$(a) \
		../../Flf/FlfCore/libSprintFlfCore.$(a)

TAGGING_TRAINER_O += $(subst src,../..,$(LIBS_SEARCH))

ifdef MODULE_CART
TAGGING_TRAINER_O += ../../Cart/libSprintCart.$(a)
endif
ifdef MODULE_MATH_NR
TAGGING_TRAINER_O += ../../Math/Nr/libSprintMathNr.$(a)
endif

# -----------------------------------------------------------------------------

build: $(TARGETS)

install:
	$(INSTALL) $(TARGETS) $(INSTALL_TARGET)

tagging-trainer$(exe): $(TAGGING_TRAINER_O)
	$(LD) $(LD_START_GROUP) $^ $(LD_END_GROUP) -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(patsubst %.o,%.d,$(filter %.o,$(TAGGING_TRAINER_O)))
