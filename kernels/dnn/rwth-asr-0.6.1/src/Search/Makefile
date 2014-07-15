#!gmake

TOPDIR		= ../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS		=
TARGETS		= libSprintSearch.$(a) check$(exe)

LIBSPRINTSEARCH_O	= \
		$(OBJDIR)/Aligner.o \
		$(OBJDIR)/LatticeHandler.o \
		$(OBJDIR)/LanguageModelLookahead.o \
		$(OBJDIR)/Module.o \
		$(OBJDIR)/Search.o \
		$(OBJDIR)/StateTree.o \
		$(OBJDIR)/StateTreeIo.o \
		$(OBJDIR)/WordConditionedTreeSearch.o

CHECK_O		= $(OBJDIR)/check.o \
			  libSprintSearch.$(a) \
			  ../Bliss/libSprintBliss.$(a) \
			  ../Fsa/libSprintFsa.$(a) \
			  ../Core/libSprintCore.$(a)



# -----------------------------------------------------------------------------

build: libSprintSearch.$(a) $(SUBDIRS)

all: $(TARGETS)

.PHONY: $(SUBDIRS)

libSprintSearch.$(a): $(LIBSPRINTSEARCH_O)
	$(MAKELIB) $@ $^

check$(exe): $(CHECK_O)
	$(LD) $(CHECK_O) -o check$(exe) $(LDFLAGS)

Wfst:
	$(MAKE) -C $@ libSprintSearchWfst.$(a)

AdvancedTreeSearch:
	$(MAKE) -C $@ libSprintAdvancedTreeSearch.$(a)


include $(TOPDIR)/Rules.make

sinclude $(LIBSPRINTSEARCH_O:.o=.d)
sinclude $(patsubst %.o,%.d,$(filter %.o,$(CHECK_O)))

# LanguageModelLookahead.cc causes an internal compiler error
# when using icpc -> compile it using g++
ifeq ($(COMPILER),icc)
$(OBJDIR)/LanguageModelLookahead.o :
	make COMPILER=gcc $@
endif
