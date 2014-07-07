#!make

TOPDIR		= ../../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS		=
TARGETS		= speech-recognizer$(exe)

COMMON_O	= ../../Speech/libSprintSpeech.$(a) \
		  ../../Lattice/libSprintLattice.$(a) \
		  ../../Am/libSprintAm.$(a) \
		  ../../Lm/libSprintLm.$(a) \
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
		  ../../Fsa/libSprintFsa.$(a)

COMMON_O += $(subst src,../..,$(LIBS_SEARCH))

ifdef MODULE_CART
COMMON_O += ../../Cart/libSprintCart.$(a)
endif
ifdef MODULE_FLF_CORE
COMMON_O += ../../Flf/FlfCore/libSprintFlfCore.$(a)
endif
ifdef MODULE_MATH_NR
COMMON_O +=	../../Math/Nr/libSprintMathNr.$(a)
endif
ifdef MODULE_NN
COMMON_O += ../../Nn/libSprintNn.$(a)
endif

# -----------------------------------------------------------------------------

SPEECH_RECOGNIZER_O = $(OBJDIR)/SpeechRecognizer.o \
		      $(COMMON_O)

FSA_SEARCH_BUILDER_O = $(OBJDIR)/FsaSearchBuilder.o \
			  $(COMMON_O)

$(OBJDIR)/FsaSearchBuilder.o : CCFLAGS += -Wno-sign-compare


# -----------------------------------------------------------------------------

build: $(TARGETS)

install:
	$(INSTALL) $(TARGETS) $(INSTALL_TARGET)

speech-recognizer$(exe):  $(SPEECH_RECOGNIZER_O)
	$(LD) $(LD_START_GROUP) $^ $(LD_END_GROUP) -o $@ $(LDFLAGS)

fsa-search-builder$(exe):  $(FSA_SEARCH_BUILDER_O)
	$(LD) $(LD_START_GROUP) $^ $(LD_END_GROUP) -o $@ $(LDFLAGS)

directory-speech-recognizer$(exe):  $(DIRECTORY_SPEECH_RECOGNIZER_O)
	$(LD) $(LD_START_GROUP) $^ $(LD_END_GROUP) -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(patsubst %.o,%.d,$(filter %.o,$(SPEECH_RECOGNIZER_O)))

