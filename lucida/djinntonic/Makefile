.PHONY: all tonic-common djinn tonic-img clean dist-clean

export CPU_ONLY=1

export INCLUDE_DIRS:=/usr/local/include
export LIBRARY_DIRS:=/usr/local/lib

all: tonic-common djinn tonic-img

tonic-common:
	make -C $@ all

djinn:
	make -C $@ all

tonic-img:
	make -C $@ all

clean:
	make -C tonic-common clean
	make -C djinn clean
	make -C tonic-img clean
