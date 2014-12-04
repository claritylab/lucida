CC = g++
CFLAGS = -O1 -lpthread

# File names
EXEC = dnn
SOURCES = $(wildcard *.cpp)
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
