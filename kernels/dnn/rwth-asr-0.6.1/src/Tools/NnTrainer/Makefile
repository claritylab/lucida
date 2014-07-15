#!make

TOPDIR		= ../../..

include $(TOPDIR)/Makefile.cfg

CCFLAGS		+= -fexceptions
# -----------------------------------------------------------------------------

SUBDIRS		=
TARGETS		= nn-trainer$(exe)

NN_TRAINER_O = $(OBJDIR)/NnTrainer.o \
		../../Am/libSprintAm.$(a) \
		../../Audio/libSprintAudio.$(a) \
		../../Bliss/libSprintBliss.$(a) \
		../../Core/libSprintCore.$(a) \
		../../Flow/libSprintFlow.$(a) \
		../../Fsa/libSprintFsa.$(a) \
		../../Lattice/libSprintLattice.$(a) \
		../../Legacy/libSprintLegacy.$(a) \
		../../Lm/libSprintLm.$(a) \
		../../Math/Lapack/libSprintMathLapack.$(a) \
		../../Math/libSprintMath.$(a) \
		../../Mc/libSprintMc.$(a) \
		../../Mm/libSprintMm.$(a) \
		../../Nn/libSprintNn.$(a) \
		../../Search/libSprintSearch.$(a) \
		../../Signal/libSprintSignal.$(a) \
		../../Speech/libSprintSpeech.$(a) \

ifdef MODULE_CART
NN_TRAINER_O += ../../Cart/libSprintCart.$(a)
endif
ifdef MODULE_FLF_CORE
NN_TRAINER_O += ../../Flf/FlfCore/libSprintFlfCore.$(a)
endif
ifdef MODULE_MATH_NR
NN_TRAINER_O += ../../Math/Nr/libSprintMathNr.$(a)
endif

# -----------------------------------------------------------------------------

build: $(TARGETS)

install:
	$(INSTALL) $(TARGETS) $(INSTALL_TARGET)

nn-trainer$(exe): $(NN_TRAINER_O)
	$(LD) $(LD_START_GROUP) $^ $(LD_END_GROUP) -o $@ $(LDFLAGS)
#	$(LD) $^ -o $@ $(LDFLAGS)


include $(TOPDIR)/Rules.make

sinclude $(patsubst %.o,%.d,$(filter %.o,$(NN_TRAINER_O)))
# DO NOT DELETE
