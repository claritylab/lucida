include Makefile.config

CAFFE = /home/xiaoweiw/tools/caffe-1.0
CXXFLAGS += -I$(CAFFE)/include/ -DCPU_ONLY
LDFLAGS += $(CAFFE)/build/lib/libcaffe.so

# CUDA = /usr/local/cuda
CXXFLAGS += -std=c++11
LDFLAGS += -ljpeg

TARGET  = FACEServer 
SOURCES = $(wildcard *.cpp ../gen-cpp2/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) -Wall $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJECTS) $(TARGET)

.PHONY: all clean
