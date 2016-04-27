CXX = g++
PROTO = protoc
CPP_CODE_FILES := $(sort $(wildcard *.cpp) $(wildcard *.cc))
CPP_OBJ_FILES := $(addsuffix .o,$(basename $(CPP_CODE_FILES)))
CXXFLAGS = -std=c++11

all: CXXFLAGS += -O3
all: proto $(CPP_OBJ_FILES)

debug: CXXFLAGS += -g3
debug: proto $(CPP_OBJ_FILES)

proto:
	$(PROTO) --proto_path=$(shell pwd) --cpp_out=$(shell pwd) $(wildcard $(shell pwd)/*.proto)

.PHONY: all debug proto clean

clean:
	-rm -rf *.o
