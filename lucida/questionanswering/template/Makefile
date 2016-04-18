CXX = g++
CXX_FLAGS = -Wall

LINK_FLAGS =  -lrt \
							-lboost_program_options \
							-lboost_filesystem \
							-lboost_system \
							-lprotobuf \
							-lthrift

TARGET_CLIENT = qaclient 

OBJECTS_CLIENT = QAService.o QAClient.o 

all: $(TARGET_CLIENT)

$(TARGET_CLIENT): $(OBJECTS_CLIENT)
	$(CXX) $(OBJECTS_CLIENT) -o $(TARGET_CLIENT) $(LINK_FLAGS)

QAService.o: gen-cpp/QAService.cpp
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

QAClient.o: QAClient.cpp
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

clean:
	rm -rf $(TARGET_CLIENT) *.o gen-* *.class

.PHONY:	all
