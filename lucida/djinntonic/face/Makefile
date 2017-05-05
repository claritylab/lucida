include Makefile.config

CAFFE = ../tools/caffe
#CAFFE = /home/xiaoweiw/tools/caffe-1.0
CXXFLAGS += -I$(CAFFE)/distribute/include/ -DCPU_ONLY
#CXXFLAGS += -I$(CAFFE)/include/ -DCPU_ONLY
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

start_server: 
	gnome-terminal -x bash -c "./FACEServer $(filter-out $@,$(MAKECMDGOALS)); read -n1"

start_test: 
	gnome-terminal -x bash -c "./run-test.sh $(filter-out $@,$(MAKECMDGOALS)); read -n1"


clean:
	$(RM) $(OBJECTS) $(TARGET)

.PHONY: all start_server start_test clean
