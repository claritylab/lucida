CXX = g++
LINK_FLAGS = -lpthread -lrt -lm
CXX_FLAGS += -O3

INPUT = ../input/gmm_data.txt
THREADS = 1

# File names
EXEC = gmm_scoring
SOURCES = gmm_scoring.cpp ../../timer/timer.cpp
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all

all : $(EXEC)
$(EXEC): $(OBJECTS) Makefile
	$(CXX) $(OBJECTS) -o $(EXEC) $(LINK_FLAGS)

# To obtain object files
%.o: %.cpp Makefile
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

# test
test: all
	@./$(EXEC) $(THREADS) $(INPUT) > /dev/null

# To remove generated files
clean:
	rm -f $(EXEC) *.o
