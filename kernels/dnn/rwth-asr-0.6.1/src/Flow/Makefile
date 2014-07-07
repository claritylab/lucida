#!gmake

TOPDIR		= ../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS		=
TARGETS		= libSprintFlow.$(a) check$(exe)

LIBSPRINTFLOW_O	= $(OBJDIR)/AbstractNode.o \
		  $(OBJDIR)/Aggregate.o \
		  $(OBJDIR)/Attributes.o \
		  $(OBJDIR)/Cache.o \
		  $(OBJDIR)/Cutter.o \
		  $(OBJDIR)/CorpusKeyMap.o \
		  $(OBJDIR)/Data.o \
		  $(OBJDIR)/DataAdaptor.o \
		  $(OBJDIR)/Datatype.o \
		  $(OBJDIR)/Demo.o \
		  $(OBJDIR)/Dump.o \
		  $(OBJDIR)/Link.o \
		  $(OBJDIR)/Module.o \
		  $(OBJDIR)/Network.o \
		  $(OBJDIR)/NetworkParser.o \
		  $(OBJDIR)/Node.o \
		  $(OBJDIR)/Registry.o \
		  $(OBJDIR)/Repeater.o \
		  $(OBJDIR)/SequenceFilter.o \
		  $(OBJDIR)/StringExpressionNode.o \
		  $(OBJDIR)/Synchronization.o \
		  $(OBJDIR)/Timestamp.o \
		  $(OBJDIR)/Vector.o \
		  $(OBJDIR)/VectorTextInput.o

CHECK_O		= $(OBJDIR)/check.o \
                  libSprintFlow.$(a) \
                  ../Core/libSprintCore.$(a)

# -----------------------------------------------------------------------------

all: $(TARGETS)

libSprintFlow.$(a): $(LIBSPRINTFLOW_O)
	$(MAKELIB) $@ $^

check$(exe): $(CHECK_O)
	$(LD) $(CHECK_O) -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(LIBSPRINTFLOW_O:.o=.d)
sinclude $(patsubst %.o,%.d,$(filter %.o,$(CHECK_O)))

