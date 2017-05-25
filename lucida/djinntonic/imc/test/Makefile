CXX = g++

CXXFLAGS = 		-std=c++11 \
				-fPIC
CXXFLAGS += $(shell if [ `lsb_release -a 2>/dev/null | grep -Poe "(?<=\s)\d+(?=[\d\.]+$$)"` -gt 14 ]; then echo "-std=c++14"; fi;)

LINKFLAGS =     -lopencv_core \
				-lopencv_highgui \
				-lopencv_imgproc \
				-lopencv_nonfree \
				-lopencv_flann \
				-lopencv_objdetect \
				-lopencv_features2d \
				-lopencv_gpu \
				-lrt \
				-lprotobuf \
				-ltesseract \
				-pthread \
				-lmongoclient \
				-lboost_program_options \
				-lboost_filesystem \
				-lboost_system \
				-lboost_thread \
				-lboost_regex \
				-lthrift \
				-lfolly \
				-lwangle \
				-lzstd \
				-lglog \
				-lthriftcpp2 \
				-lgflags \
				-lthriftprotocol \
				-lssl \
				-lcrypto

TARGET  = IMCClient
SOURCES = gen-cpp2/LucidaService_client.cpp \
				gen-cpp2/lucidaservice_constants.cpp \
				gen-cpp2/LucidaService.cpp \
				gen-cpp2/LucidaService_processmap_binary.cpp \
				gen-cpp2/LucidaService_processmap_compact.cpp \
				gen-cpp2/lucidaservice_types.cpp \
				gen-cpp2/lucidatypes_constants.cpp \
				gen-cpp2/lucidatypes_types.cpp \
				$(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

all: CXXFLAGS += -O3
all: thrift $(TARGET)

debug: CXXFLAGS += -g3
debug: thrift $(TARGET)

thrift:
	@if [ ! -d "gen-cpp2" ]; then \
       python -mthrift_compiler.main --gen cpp2 ../../../lucidaservice.thrift; \
       python -mthrift_compiler.main --gen cpp2 ../../../lucidatypes.thrift; \
    fi

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LINKFLAGS) -o $@

%.o: %.cpp
	$(CXX) -Wall $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET) *.o gen-cpp2

.PHONY:	all debug thrift clean
