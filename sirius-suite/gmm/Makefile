PLATFORMS = baseline pthread
ifneq (($shell which nvcc),)
PLATFORMS += gpu
endif

.PHONY: all $(PLATFORMS)

SUBCLEAN = $(addsuffix .clean, $(PLATFORMS))
.PHONY: clean $(SUBCLEAN)

SUBTEST = $(addsuffix .test, $(PLATFORMS))
.PHONY: test $(SUBTEST)

all: $(PLATFORMS)
$(PLATFORMS):
	$(MAKE) -C $@

clean: $(SUBCLEAN)
$(SUBCLEAN): %.clean:
	$(MAKE) -C $* clean

test: $(SUBTEST)
$(SUBTEST): %.test:
	$(MAKE) -C $* test
