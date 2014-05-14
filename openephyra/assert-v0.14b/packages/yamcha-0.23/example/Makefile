CFLAGS = -O3 -Wall `yamcha-config --cflags`
LIBS = `yamcha-config --libs`
CXXC   = g++
CC     = gcc

.cc.o:
	$(CXXC) $(CFLAGS) $(INC) $(DEFS) -c $<

.c.o:
	$(CC) $(CFLAGS) $(INC) $(DEFS) -c  $<
		
all:  example

example:  example.o
	$(CXXC) -o $@ example.o $(LIBS)

clean:
	rm  -f *.o example
