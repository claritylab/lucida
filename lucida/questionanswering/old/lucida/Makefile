CXX = g++
CXX_FLAGS = -Wall

LINK_FLAGS = -lrt \
						 -lboost_program_options \
						 -lboost_filesystem \
						 -lboost_system \
						 -lprotobuf \
						 -lthrift

all: gen-cpp/QAService.o java.deps

java.deps:
	./compile-qa.sh

gen-cpp/QAService.o: gen-cpp/QAService.cpp
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

clean:
	rm -rf *.o gen-* *.class

.PHONY:	all
