# Makefile for generating the kaldi-thrift service
CXX = g++
FLAGS = --std=c++11
TARGET_CLIENT = client
LINK_FLAGS = -lthrift -lsox

# Needed for finding header files during preprocessing
OBJECTS_CLIENT = gen-cpp/LucidaService.o gen-cpp/lucidatypes_types.cpp Kaldi_Client.o

debug: FLAGS += -g3 -DDEBUG
debug: clean all

all: thrift $(TARGET_CLIENT)

$(TARGET_CLIENT): $(OBJECTS_CLIENT)
	$(CXX) $(FLAGS) $(OBJECTS_CLIENT) -o $(TARGET_CLIENT) $(LINK_FLAGS)

thrift:
	thrift --gen cpp lucidaservice.thrift
	thrift --gen cpp lucidatypes.thrift

%.o: %.cpp
	$(CXX) $(FLAGS) -c $(CXX_FLAGS) $< -o $@

clean:
	rm -rf $(TARGET_CLIENT) gen-cpp *.o *~
