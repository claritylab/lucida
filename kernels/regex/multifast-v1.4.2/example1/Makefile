APP_NAME := example1
INCLUDE_DIRECTORY := ../ahocorasick
LIBRARY_DIRECTORY := ../ahocorasick/build
LINK_LIBRARY := ahocorasick
BUILD_DIRECTORY := build

ifeq ($(wildcard $(LIBRARY_DIRECTORY)/lib$(LINK_LIBRARY).a),) 
all:;@echo 'Please go to ../ahocorasick directory and complie it first.'
else
all: $(APP_NAME)
endif

$(APP_NAME): mkbuilddir $(APP_NAME).o
	cc -o $(BUILD_DIRECTORY)/$(APP_NAME) $(BUILD_DIRECTORY)/$(APP_NAME).o -l$(LINK_LIBRARY) -L$(LIBRARY_DIRECTORY)

$(APP_NAME).o: $(APP_NAME).c
	cc -o $(BUILD_DIRECTORY)/$(APP_NAME).o -I$(INCLUDE_DIRECTORY) -Wall -c $(APP_NAME).c

mkbuilddir:
	mkdir -p $(BUILD_DIRECTORY)

clean:
	rm -rf $(BUILD_DIRECTORY)