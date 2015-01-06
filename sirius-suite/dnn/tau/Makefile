include ../compile.mk

all: dnn

dnn: dnn.o
	${CXX} ${LDFLAGS} -o $@ $^
	cp -f dnn ${INSTALLDIR}

install: 
	cp -f dnn ${INSTALLDIR}

clean: 
	rm -f *.o dnn
