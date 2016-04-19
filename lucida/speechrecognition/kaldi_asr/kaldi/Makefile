SUBDIRS=
include ../../../../Makefile.common

all-here::
	@if [ ! -d "src" ]; then \
        cd scripts && ./prepare.sh; \
	fi
	cd scripts && ./compile-kaldi.sh

clean-here::
	cd scripts && ./clean.sh && cd -
