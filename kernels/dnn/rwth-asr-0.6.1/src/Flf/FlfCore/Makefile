#!gmake

TOPDIR		= ../../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS 	=
TARGETS		= libSprintFlfCore.$(a) check$(exe)

#check$(exe)

LIBSPRINTFLFCORE_O = $(OBJDIR)/Ftl.o \
		  $(OBJDIR)/Basic.o \
		  $(OBJDIR)/Boundaries.o \
		  $(OBJDIR)/Lattice.o \
		  $(OBJDIR)/LatticeInternal.o \
		  $(OBJDIR)/Semiring.o \
		  $(OBJDIR)/TopologicalOrderQueue.o \
		  $(OBJDIR)/Traverse.o \
		  $(OBJDIR)/Types.o \
		  $(OBJDIR)/Utility.o \
		  $(OBJDIR)/Weight.o

CHECK_O		= $(OBJDIR)/check.o \
		  libSprintFlfCore.$(a) \
		  ../../Fsa/libSprintFsa.$(a) \
		  ../../Core/libSprintCore.$(a)

# -----------------------------------------------------------------------------

all: $(TARGETS)

libSprintFlfCore.$(a): $(LIBSPRINTFLFCORE_O)
	$(MAKELIB) $@ $^

check$(exe): $(CHECK_O)
	$(LD) $^ -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(LIBSPRINTFLFCORE_O:.o=.d)
sinclude $(patsubst %.o,%.d,$(filter %.o,$(CHECK_O)))
