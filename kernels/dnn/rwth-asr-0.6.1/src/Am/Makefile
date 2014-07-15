#!gmake

TOPDIR		= ../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS 	=
TARGETS		= libSprintAm.$(a) check$(exe)

LIBSPRINTAM_O	= \
		$(OBJDIR)/AcousticModel.o \
		$(OBJDIR)/AcousticModelAdaptor.o \
		$(OBJDIR)/ClassicAcousticModel.o \
		$(OBJDIR)/ClassicHmmTopologySet.o \
		$(OBJDIR)/ClassicStateModel.o \
		$(OBJDIR)/ClassicStateTying.o \
		$(OBJDIR)/ClassicTransducerBuilder.o \
		$(OBJDIR)/Module.o \
		$(OBJDIR)/TransitionModel.o \
		$(OBJDIR)/Utilities.o

CHECK_O		= $(OBJDIR)/check.o \
		  libSprintAm.$(a) \
		  ../Mm/libSprintMm.$(a) \
		  ../Legacy/libSprintLegacy.$(a) \
		  ../Bliss/libSprintBliss.$(a) \
		  ../Fsa/libSprintFsa.$(a) \
		  ../Core/libSprintCore.$(a) \
		  ../Mc/libSprintMc.$(a) \
                  ../Math/libSprintMath.$(a) \
		  ../Math/Lapack/libSprintMathLapack.$(a)


ifdef MODULE_CART
LIBSPRINTAM_O  += $(OBJDIR)/DecisionTreeStateTying.o
LIBSPRINTAM_O  += $(OBJDIR)/ClassicDecisionTree.o
CHECK_O        += ../Cart/libSprintCart.$(a)
endif

ifdef MODULE_MATH_NR
CHECK_O        += ../Math/Nr/libSprintMathNr.$(a)
endif

ifdef MODULE_ADAPT_MLLR
LIBSPRINTAM_O  += $(OBJDIR)/AdaptedAcousticModel.o
LIBSPRINTAM_O  += $(OBJDIR)/AdaptationTree.o
endif


# -----------------------------------------------------------------------------

all: $(TARGETS)

libSprintAm.$(a): $(LIBSPRINTAM_O)
	$(MAKELIB) $@ $^

check$(exe): $(CHECK_O)
	$(LD) $^ -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(LIBSPRINTAM_O:.o=.d)
sinclude $(patsubst %.o,%.d,$(filter %.o,$(CHECK_O)))
