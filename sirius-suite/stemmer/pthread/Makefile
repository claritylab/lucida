CXX = g++
LINK_FLAGS = -lrt -lpthread
CXX_FLAGS  = -O3 -fpermissive -Wno-write-strings

THREADS = 4
INPUT   = ../input/voc.txt

# File names
EXEC = stem_porter
SOURCES = main.cpp porter.cpp ../../utils/timer.cpp
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all

all : $(EXEC)
$(EXEC): $(OBJECTS) Makefile
	$(CXX) $(CXX_FLAGS) $(OBJECTS) -o $(EXEC) $(LINK_FLAGS)

# To obtain object files
%.o: %.cpp Makefile
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

# test
test: CXX_FLAGS += -DTESTING
test: all
	@./$(EXEC) $(THREADS) $(INPUT)
	@diff ../input/stemmer.baseline ../input/stemmer.pthread > /dev/null ; \
		if [ $$? -eq 1 ] ; then echo "STEMMER-PTHREAD test failed" ; fi

# To remove generated files
clean:
	rm -f $(EXEC) *.o
