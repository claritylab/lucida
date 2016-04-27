CXX = g++
CXX_FLAGS = -std=c++11
TARGET_CLIENT = asr_client
LINK_FLAGS = -lthrift -lsox

OBJECTS_CLIENT = gen-cpp/LucidaService.o gen-cpp/lucidatypes_types.o asr_client.o

all: CXX_FLAGS += -O3
all: thrift $(TARGET_CLIENT)

debug: CXX_FLAGS += -g3
debug: thrift $(TARGET_CLIENT)

$(TARGET_CLIENT): $(OBJECTS_CLIENT)
	$(CXX) -g $(CXX_FLAGS) $(OBJECTS_CLIENT) -o $(TARGET_CLIENT) $(LINK_FLAGS)

thrift:
	@if [ ! -d "gen-cpp" ]; then \
       thrift --gen cpp ../../../lucidaservice.thrift; \
       thrift --gen cpp ../../../lucidatypes.thrift; \
    fi

%.o: %.cpp
	$(CXX) -g $(CXX_FLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET_CLIENT) gen-cpp *.o *~

.PHONY:	all debug thrift clean
