include ../compile.mk

all: regex

regex: slre.o
	${CXX} ${LDFLAGS} -o $@ $^
	cp -f regex ${INSTALLDIR}

install: 
	cp -f regex ${INSTALLDIR}

clean: 
	rm -f *.o regex
