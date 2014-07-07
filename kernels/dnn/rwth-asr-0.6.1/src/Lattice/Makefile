#!gmake

TOPDIR		= ../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS 		=
#TARGETS			= libSprintLattice.$(a) check$(exe)
TARGETS			= libSprintLattice.$(a)

LIBSPRINTLATTICE_O	= \
			  $(OBJDIR)/Archive.o \
			  $(OBJDIR)/Basic.o \
			  $(OBJDIR)/Lattice.o \
			  $(OBJDIR)/LatticeAdaptor.o \
			  $(OBJDIR)/Morphism.o \
			  $(OBJDIR)/Utilities.o

CHECK_O			= $(OBJDIR)/check.o \
			  libSprintLattice.$(a) \
			  ../Speech/libSprintSpeech.$(a) \
			  ../Lm/libSprintLm.$(a) \
			  ../Am/libSprintAm.$(a) \
			  ../Mm/libSprintMm.$(a) \
			  ../Mc/libSprintMc.$(a) \
			  ../Legacy/libSprintLegacy.$(a) \
			  ../Bliss/libSprintBliss.$(a) \
			  ../Fsa/libSprintFsa.$(a) \
			  ../Audio/libSprintAudio.$(a) \
			  ../Signal/libSprintSignal.$(a) \
			  ../Flow/libSprintFlow.$(a) \
			  ../Math/libSprintMath.$(a) \
			  ../Math/Lapack/libSprintMathLapack.$(a) \
			  ../Core/libSprintCore.$(a) \
			  ../Cart/libSprintCart.$(a)


ifdef MODULE_LATTICE_BASIC
LIBSPRINTLATTICE_O += $(OBJDIR)/Cache.o
LIBSPRINTLATTICE_O += $(OBJDIR)/Static.o
LIBSPRINTLATTICE_O += $(OBJDIR)/Compose.o
endif

ifdef MODULE_LATTICE_DT
LIBSPRINTLATTICE_O += $(OBJDIR)/Accuracy.o
LIBSPRINTLATTICE_O += $(OBJDIR)/Arithmetic.o
LIBSPRINTLATTICE_O += $(OBJDIR)/Best.o
LIBSPRINTLATTICE_O += $(OBJDIR)/Merge.o
LIBSPRINTLATTICE_O += $(OBJDIR)/Posterior.o
LIBSPRINTLATTICE_O += $(OBJDIR)/Rational.o
LIBSPRINTLATTICE_O += $(OBJDIR)/RemoveEpsilons.o
#MODF Accumulator.hh
#MODF Accumulator.tcc
endif



ifdef MODULE_LATTICE_HTK
LIBSPRINTLATTICE_O += $(OBJDIR)/HtkReader.o
LIBSPRINTLATTICE_O += $(OBJDIR)/HtkWriter.o
endif


ifdef MODULE_MATH_NR
CHECK_O += ../Math/Nr/libSprintMathNr$(a)
endif


# -----------------------------------------------------------------------------

all: $(TARGETS)

libSprintLattice.$(a): $(LIBSPRINTLATTICE_O)
	$(MAKELIB) $@ $^

check$(exe): $(CHECK_O)
	$(LD) $^ -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(LIBSPRINTLATTICE_O:.o=.d)
sinclude $(patsubst %.o,%.d,$(filter %.o,$(CHECK_O)))
