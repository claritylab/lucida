# Makefile for generating the kaldi-thrift service
CXX = g++
FLAGS = --std=c++11
TARGET_CLIENT = client
TARGET_SERVER = server
LINK_FLAGS = -lthrift -lsox

# Needed for finding header files during preprocessing
OBJECTS_CLIENT = gen-cpp/KaldiService.o Kaldi_Client.o
OBJECTS_SERVER = gen-cpp/KaldiService.o Kaldi_Server.o 

all: thrift $(TARGET_SERVER) $(TARGET_CLIENT)

$(TARGET_CLIENT): $(OBJECTS_CLIENT)
	$(CXX) $(FLAGS) $(OBJECTS_CLIENT) -o $(TARGET_CLIENT) $(LINK_FLAGS)

$(TARGET_SERVER): $(OBJECTS_SERVER)
	$(CXX) $(FLAGS) $(OBJECTS_SERVER) -o $(TARGET_SERVER) $(LINK_FLAGS)

thrift:
	thrift --gen cpp kaldi.thrift

%.o: %.cpp
	$(CXX) $(FLAGS) -c $(CXX_FLAGS) $< -o $@

clean:
	rm -rf $(TARGET_CLIENT) $(TARGET_SERVER) gen-cpp *.o *~ *.wav

.PHONY:	all
