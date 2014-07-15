#!gmake

TOPDIR		= ../../..

include $(TOPDIR)/Makefile.cfg

#CPPFLAGS	+= -I$(QTDIR)/include -DQT_THREAD_SUPPORT -DQT_CLEAN_NAMESPACE
#LDFLAGS	+= -L$(QTDIR)/lib -lqt-mt

# -----------------------------------------------------------------------------

SUBDIRS		=
TARGETS		= feature-extraction$(exe)

FEATURE_EXTRACTION_O = $(OBJDIR)/FeatureExtraction.o \
		  $(OBJDIR)/FeatureExtractor.o \
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
		  ../../Core/libSprintCore.$(a) \
		  ../../Fsa/libSprintFsa.$(a) \

FEATURE_EXTRACTION_O += $(subst src,../..,$(LIBS_SEARCH))

ifdef MODULE_CART
FEATURE_EXTRACTION_O += ../../Cart/libSprintCart.$(a)
endif
ifdef MODULE_FLF_CORE
FEATURE_EXTRACTION_O += ../../Flf/FlfCore/libSprintFlfCore.$(a)
endif
ifdef MODULE_MATH_NR
FEATURE_EXTRACTION_O +=	../../Math/Nr/libSprintMathNr.$(a)
endif
ifdef MODULE_NN
FEATURE_EXTRACTION_O +=	../../Nn/libSprintNn.$(a)
endif


# -----------------------------------------------------------------------------

build: $(TARGETS)

install:
	$(INSTALL) $(TARGETS) $(INSTALL_TARGET)

feature-extraction$(exe): $(FEATURE_EXTRACTION_O)
	$(LD) $(LD_START_GROUP) $^ $(LD_END_GROUP) -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(patsubst %.o,%.d,$(filter %.o,$(FEATURE_EXTRACTION_O)))
