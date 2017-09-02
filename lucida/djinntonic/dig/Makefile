include Makefile.config

CAFFE = ../tools/caffe
#CAFFE = /home/xiaoweiw/tools/caffe-1.0
CXXFLAGS += -I/usr/include/hdf5/serial -I$(CAFFE)/distribute/include/ -DCPU_ONLY
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

CXXFLAGS += $(shell if [ `lsb_release -a 2>/dev/null | grep -Poe "(?<=\s)\d+(?=[\d\.]+$$)"` -gt 14 ]; then echo "-std=c++14"; fi;)
LDFLAGS += -ljpeg -lzstd

TARGET  = DIGServer
SOURCES = $(wildcard *.cpp ../gen-cpp2/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET) client

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) -Wall $(CXXFLAGS) -c $< -o $@

start_server: 
	./DIGServer

start_test: 
	cd test && ./DIGClient && cd ..

client:
	cd test && make all && cd ..

clean:
	$(RM) $(OBJECTS) $(TARGET) && cd test && make clean && cd ..

.PHONY: all start_server start_test clean
