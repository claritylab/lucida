CXX = g++
LINK_FLAGS = -lpthread -lrt -lm
CXX_FLAGS = -O3

THREADS   = 4
LIST      = ../input/list
QUESTIONS = ../input/questions

# File names
EXEC = regex_slre
SOURCES = main.cpp slre.cpp ../../timer/timer.cpp
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all

all : $(EXEC)
$(EXEC): $(OBJECTS) Makefile
	$(CXX) $(CXX_FLAGS) $(OBJECTS) -o $(EXEC) $(LINK_FLAGS)

# To obtain object files
%.o: %.cpp Makefile
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

# test
test: all
	@./$(EXEC) $(THREADS) $(LIST) $(QUESTIONS)

# To remove generated files
clean:
	rm -f $(EXEC) *.o
