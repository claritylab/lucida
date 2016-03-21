KERNELS = gmm dnn-asr fe fd crf regex stemmer

.PHONY: all $(KERNELS)

SUBCLEAN = $(addsuffix .clean, $(KERNELS))
.PHONY: clean $(SUBCLEAN)

SUBTEST = $(addsuffix .test, $(KERNELS))
.PHONY: test $(SUBTEST)

all: $(KERNELS) prepare

prepare:
	./prepare.sh

$(KERNELS):
	$(MAKE) -C $@

clean: $(SUBCLEAN)
$(SUBCLEAN): %.clean:
	$(MAKE) -C $* clean

test: $(SUBTEST)
$(SUBTEST): %.test:
	@$(MAKE) -s -C $* test
