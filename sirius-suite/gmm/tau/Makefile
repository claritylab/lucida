include ../compile.mk

all: gmm_scoring

gmm_scoring: gmm_scoring.o
	${CXX} ${LDFLAGS} -o $@ $^
	cp -f gmm_scoring ${INSTALLDIR}

install: 
	cp -f gmm_scoring ${INSTALLDIR}

clean: 
	rm -f *.o gmm_scoring
