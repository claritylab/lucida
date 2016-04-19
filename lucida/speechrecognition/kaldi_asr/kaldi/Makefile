SUBDIRS=
include ../../../../Makefile.common

all-here::
	if [ ! -d "src" ]; then \
        cd scripts && ./prepare.sh && ./compile-kaldi.sh; \
    fi

clean-here::
	cd scripts && ./clean.sh 
