include Makefile.config

CAFFE = ../tools/caffe
#CAFFE = /home/xiaoweiw/tools/caffe-1.0
CXXFLAGS += -I/usr/include/hdf5/serial -I$(CAFFE)/distribute/include/
#CXXFLAGS += -I$(CAFFE)/include/ -DCPU_ONLY
NVCC_RESULT := $(shell which nvcc 2> /dev/null)
NVCC_TEST := $(notdir $(NVCC_RESULT))
ifneq ($(NVCC_TEST),nvcc)
	CXXFLAGS += -DCPU_ONLY
else
	CXXFLAGS += -I/usr/local/cuda/include
	LDFLAGS += -L/usr/local/cuda/lib64 -L/usr/local/cuda/lib32 -L/usr/local/cuda/lib
endif
LDFLAGS += $(CAFFE)/build/lib/libcaffe.so

CXXFLAGS += -std=c++14
LDFLAGS += -ljpeg -lzstd

TARGET  = IMCServer
SOURCES = $(wildcard *.cpp ../gen-cpp2/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) -Wall $(CXXFLAGS) -c $< -o $@

start_server: 
	gnome-terminal -x bash -c "./IMCServer $(filter-out $@,$(MAKECMDGOALS)); read -n1"

start_test: 
	gnome-terminal -x bash -c "./run-test.sh $(filter-out $@,$(MAKECMDGOALS)); read -n1"


clean:
	$(RM) $(OBJECTS) $(TARGET)

.PHONY: all start_server start_test clean
