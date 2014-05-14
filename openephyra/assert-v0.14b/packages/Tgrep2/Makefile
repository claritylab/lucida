# Edit this line to point to the directory containing the drutils.h file:
UTIL_DIR=./DRUtils

UTIL_LIB= ${UTIL_DIR}/${HOSTTYPE}/libdrutils.a
CC= gcc -O4 -Wall
#CC= gcc -g -Wall

all: drutils tgrep2

drutils: 
		cd $(UTIL_DIR); make clean; make

tgrep2: drutils tgrep2.c Makefile
	${CC} -o $@ -I${UTIL_DIR} tgrep2.c ${UTIL_LIB} -lm

clean:
	rm -f *.o tgrep2
