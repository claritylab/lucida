include ../Makefile.common

all-here::
	for i in $(wildcard install_*) ; do \
		echo $$i ; \
		./$$i ; \
	done

clean-here::
	./clean.sh
