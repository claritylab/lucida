PLATFORMS = baseline pthread

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
	$(MAKE) -s -C $* test
