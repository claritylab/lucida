CXX = g++
LINK_FLAGS = -lpthread -lrt -lm
CXX_FLAGS = -O3

INPUT = ../input/gmm_data.txt
THREADS = 4

# File names
EXEC = gmm_scoring
SOURCES = gmm_scoring.cpp ../../utils/timer.cpp ../../utils/correct.cpp
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all

all : $(EXEC)
$(EXEC): $(OBJECTS) Makefile
	$(CXX) $(OBJECTS) -o $(EXEC) $(LINK_FLAGS)

# To obtain object files
%.o: %.cpp Makefile
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

# test
test: CXX_FLAGS += -DTESTING
test: all
	@./$(EXEC) $(THREADS) $(INPUT)
	diff ../input/gmm.baseline ../input/gmm.pthread > /dev/null
		if [ $$? -eq 1 ] ; then echo "GMM-Pthread test failed" ; fi

# To remove generated files
clean:
	rm -f $(EXEC) *.o
