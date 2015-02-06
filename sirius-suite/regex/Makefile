PLATFORMS = baseline gpu pthread
BUILD = $(PLATFORMS:%=build-%)

.PHONY: platforms $(PLATFORMS)

platforms: $(PLATFORMS)

$(PLATFORMS):
	$(MAKE) -C $@
