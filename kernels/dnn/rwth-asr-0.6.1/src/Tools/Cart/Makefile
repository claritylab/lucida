#!gmake

TOPDIR		= ../../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS		=
TARGETS		= cart-viewer$(exe) cart-trainer$(exe)

CART_VIEWER_O		= $(OBJDIR)/CartViewer.o \
		  ../../Cart/libSprintCart.$(a) \
		  ../../Core/libSprintCore.$(a)

CART_TRAINER_O	= $(OBJDIR)/CartTrainer.o \
		  ../../Cart/libSprintCart.$(a) \
		  ../../Core/libSprintCore.$(a)


# -----------------------------------------------------------------------------

build: $(TARGETS)

install:
	$(INSTALL) $(TARGETS) $(INSTALL_TARGET)

cart-viewer$(exe): $(CART_VIEWER_O)
	$(LD) $^ -o $@ $(LDFLAGS)

cart-trainer$(exe): $(CART_TRAINER_O)
	$(LD) $^ -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(patsubst %.o,%.d,$(filter %.o,$(CART_VIEWER_O)))
sinclude $(patsubst %.o,%.d,$(filter %.o,$(CART_TRAINER_O)))
