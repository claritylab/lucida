#!gmake

TOPDIR		= ../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS 	=
TARGETS		= libSprintLegacy.$(a) #check$(exe)

LIBLEGACY_O	= \
		$(OBJDIR)/DecisionTree.o



CHECK_O		= \
		$(OBJDIR)/check.o			\
		libSprintLegacy.$(a)			\
		../Audio/libSprintAudio.$(a)		\
		../Bliss/libSprintBliss.$(a)		\
		../Speech/libSprintSpeech.$(a)		\
		../Flow/libSprintFlow.$(a)		\
		../Core/libSprintCore.$(a)		\
		../Fsm/libSprintFsm.$(a)		\
		../Lm/libSprintLm.$(a)			\
		../Mm/libSprintMm.$(a)			\
		../Signal/libSprintSignal.$(a)		\



# -----------------------------------------------------------------------------

all: $(TARGETS)


libSprintLegacy.$(a): $(LIBLEGACY_O)
	$(MAKELIB) $@ $^

check$(exe): $(CHECK_O)
	$(LD) $^ -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(LIBLEGACY_O:.o=.d)
sinclude $(patsubst %.o,%.d,$(filter %.o,$(CHECK_O)))
