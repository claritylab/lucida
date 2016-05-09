CXX = g++

CXXFLAGS = -std=c++11 \
				-fPIC

LINKFLAGS =  -lboost_program_options \
				-lboost_filesystem \
				-lboost_system \
				-lboost_thread \
				-lboost_regex \
				-lthrift \
				-lfolly \
				-lwangle \
				-lglog \
				-lthriftcpp2 \
				-lgflags \
				-lthriftprotocol \
				-lcrypto

TARGET  = cmd
SOURCES = gen-cpp2/LucidaService_client.cpp \
				gen-cpp2/lucidaservice_constants.cpp \
				gen-cpp2/LucidaService.cpp \
				gen-cpp2/LucidaService_processmap_binary.cpp \
				gen-cpp2/LucidaService_processmap_compact.cpp \
				gen-cpp2/lucidaservice_types.cpp \
				gen-cpp2/lucidatypes_constants.cpp \
				gen-cpp2/lucidatypes_types.cpp \
				$(wildcard *.cpp) \
OBJECTS = $(SOURCES:.cpp=.o)

all: CXXFLAGS += -O3
all: thrift $(TARGET)

debug: CXXFLAGS += -g3
debug: thrift $(TARGET)

thrift:
	@if [ ! -d "gen-cpp2" ]; then \
	   thrift1 --gen py ../../../lucidaservice.thrift; \
       python -mthrift_compiler.main --gen cpp2 ../../../lucidaservice.thrift; \
       python -mthrift_compiler.main --gen cpp2 ../../../lucidatypes.thrift; \
    fi

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LINKFLAGS)

%.o: %.cpp
	$(CXX) -Wall $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET) *.o gen-cpp2

start_server: 
	gnome-terminal -x bash -c "./cmd $(filter-out $@,$(MAKECMDGOALS)); read -n1"

.PHONY:	all debug thrift clean start_server
