#!gmake

TOPDIR		= ../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS		=
TARGETS		= libSprintCart.$(a) check$(exe)

LIBSPRINTCART_O	= \
		$(OBJDIR)/Conditions.o          \
		$(OBJDIR)/Properties.o          \
		$(OBJDIR)/Example.o             \
		$(OBJDIR)/BinaryTree.o          \
		$(OBJDIR)/DecisionTree.o        \
		$(OBJDIR)/Cluster.o             \
		$(OBJDIR)/DecisionTreeTrainer.o \
		$(OBJDIR)/Parser.o

CHECK_O	        = $(OBJDIR)/check.o \
		  libSprintCart.$(a) \
		  ../Core/libSprintCore.$(a)

# -----------------------------------------------------------------------------

all: $(TARGETS)

libSprintCart.$(a): $(LIBSPRINTCART_O)
	$(MAKELIB) $@ $^

check$(exe): $(CHECK_O)
	$(LD) $(CHECK_O) -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(LIBSPRINTCART_O:.o=.d)
sinclude $(patsubst %.o,%.d,$(filter %.o,$(CHECK_O)))
