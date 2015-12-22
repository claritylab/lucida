CXX = g++
CXX_FLAGS = -O2

LINK_FLAGS =    -L$(BOOST_LIB) \
				-lboost_program_options \
				-lboost_filesystem \
				-lboost_system \
				-lboost_thread \
				-lboost_regex \
				-lthrift

TARGET_SERVER = ccserver
TARGET_CLIENT = ccclient

QA_DEPENDENCY = ../qa/lucida/
ASR_DEPENDENCY = ../asr/lucida/
IMM_DEPENDENCY = ../imm/lucida/

OBJECTS_SERVER = gen-cpp/QAService.o gen-cpp/KaldiService.o gen-cpp/ImageMatchingService.o \
		gen-cpp/CommandCenter.o gen-cpp/commandcenter_types.o CmdCenterDaemon.o base64.o
OBJECTS_CLIENT = gen-cpp/CommandCenter.o gen-cpp/commandcenter_types.o CmdCenterClient.o

all: thrift $(TARGET_SERVER) $(TARGET_CLIENT)

thrift: thrift_dep thrift_cc

thrift_dep: 
	thrift --gen cpp $(QA_DEPENDENCY)qaservice.thrift
	thrift --gen cpp $(ASR_DEPENDENCY)kaldi.thrift
	thrift --gen cpp $(IMM_DEPENDENCY)service.thrift
	
thrift_cc:
	thrift --gen cpp --gen java --gen js:node commandcenter.thrift
	thrift --gen cpp --gen js:node filetransfer_svc.thrift

$(TARGET_CLIENT): $(OBJECTS_CLIENT)
	$(CXX) $(OBJECTS_CLIENT) -o $(TARGET_CLIENT) $(LINK_FLAGS)

$(TARGET_SERVER): $(OBJECTS_SERVER)
	$(CXX) $(OBJECTS_SERVER) -o $(TARGET_SERVER) $(LINK_FLAGS)

gen-cpp/%.o : gen-cpp/%.cpp
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

%.o : %.cpp
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

clean:
	rm -rf $(TARGET_SERVER) ccclient ccserver *.o gen-cpp/*.o

.PHONY:	all
