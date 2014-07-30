CC=/home/jahausw/tools/kremlin/bin/kremlin-gcc
all: regex

regex: slre.o
	${CC} ${LDFLAGS} -o $@ $^

clean: 
	rm -f *.o regex
