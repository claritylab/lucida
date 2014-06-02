APP_NAME := example3
INCLUDE_DIRECTORY := ../ahocorasick
LIBRARY_DIRECTORY := ../ahocorasick/build
LINK_LIBRARY := ahocorasick
BUILD_DIRECTORY := build

ifeq ($(wildcard $(LIBRARY_DIRECTORY)/lib$(LINK_LIBRARY).a),) 
all:;@echo 'Please go to ../ahocorasick directory and complie it first.'
else
all: $(APP_NAME)
endif

$(APP_NAME): mkbuilddir $(APP_NAME).o AhoCorasickPlus.o
	g++ -o $(BUILD_DIRECTORY)/$(APP_NAME) $(BUILD_DIRECTORY)/$(APP_NAME).o $(BUILD_DIRECTORY)/AhoCorasickPlus.o -l$(LINK_LIBRARY) -L$(LIBRARY_DIRECTORY)

$(APP_NAME).o: $(APP_NAME).cpp
	g++ -o $(BUILD_DIRECTORY)/$(APP_NAME).o -I$(INCLUDE_DIRECTORY) -Wall -c $(APP_NAME).cpp

AhoCorasickPlus.o: AhoCorasickPlus.cpp $(HERADERS)
	g++ -o $(BUILD_DIRECTORY)/AhoCorasickPlus.o -c AhoCorasickPlus.cpp $(CFLAGS) -I$(INCLUDE_DIRECTORY)

mkbuilddir:
	mkdir -p $(BUILD_DIRECTORY)

clean:
	rm -rf $(BUILD_DIRECTORY)
