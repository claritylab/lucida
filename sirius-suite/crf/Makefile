PLATFORMS = baseline pthread

.PHONY: all $(PLATFORMS)

SUBCLEAN = $(addsuffix .clean, $(PLATFORMS))
.PHONY: clean $(SUBCLEAN)

SUBTEST = $(addsuffix .test, $(PLATFORMS))
.PHONY: test $(SUBTEST)

all: $(PLATFORMS)
$(PLATFORMS):
	cd $@ && ./sirius-crf.sh 1>/dev/null && cd ..

clean: $(SUBCLEAN)
$(SUBCLEAN): %.clean:
	cd $* && rm -rf crfsuite* .libs/ && cd ..

test: $(SUBTEST)
$(SUBTEST): %.test:
	cd $* && ./test-crf.sh && cd ..
