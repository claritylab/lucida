include ../compile.mk

all: stem_tau

stem_tau: stem_tau.o
	${CXX} ${LDFLAGS} -o $@ $^
	cp -f stem_tau ${INSTALLDIR}

install: 
	cp -f stem_tau ${INSTALLDIR}

clean: 
	rm -f *.o stem_tau
