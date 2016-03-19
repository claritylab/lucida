CXX = g++
CXX_FLAGS = -O2

LINK_FLAGS = -L$(BOOST_LIB) \
						 -lboost_program_options \
						 -lboost_filesystem \
						 -lboost_system \
						 -lboost_thread \
						 -lboost_regex \
						 -lthrift

TARGET_SERVER = cmd_server
TARGET_CLIENT = ccclient

QA_DEPENDENCY  = $(LUCIDAROOT)/questionanswering/lucida
ASR_DEPENDENCY = $(LUCIDAROOT)/speechrecognition/lucida
IMM_DEPENDENCY = $(LUCIDAROOT)/imagematching/lucida

OBJECTS_SERVER = gen-cpp/QAService.o gen-cpp/KaldiService.o gen-cpp/ImageMatchingService.o \
		gen-cpp/CommandCenter.o gen-cpp/commandcenter_types.o CmdCenterDaemon.o base64.o
OBJECTS_CLIENT = gen-cpp/CommandCenter.o gen-cpp/commandcenter_types.o CmdCenterClient.o

all: $(TARGET_SERVER) $(TARGET_CLIENT)

$(TARGET_SERVER): $(OBJECTS_SERVER)
	$(CXX) $(OBJECTS_SERVER) -o $(TARGET_SERVER) $(LINK_FLAGS)

$(TARGET_CLIENT): $(OBJECTS_CLIENT)
	$(CXX) $(OBJECTS_CLIENT) -o $(TARGET_CLIENT) $(LINK_FLAGS)

gen-cpp/%.o : gen-cpp/%.cpp
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

%.o : %.cpp
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

clean:
	rm -rf $(TARGET_SERVER) ccclient ccserver *.o gen-*
