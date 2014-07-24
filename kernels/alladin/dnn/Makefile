CC = gcc
CFLAGS = -std=c89 -pedantic -ansi -O3

# File names
EXEC = dnn
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)

# Main target
$(EXEC): $(OBJECTS) Makefile
	$(CC) $(OBJECTS) -o $(EXEC) $(LINK_FLAGS) $(CFLAGS) 

# To obtain object files
%.o: %.cpp Makefile
	$(CC) -c $(CFLAGS) $(EXTRA_FLAGS) $< -o $@

# To remove generated files
clean:
	rm -f $(EXEC) *.o
