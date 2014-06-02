PROGRAM := multifast
BUILD_DIRECTORY := build
INCLUDE_DIRECTORY := ../ahocorasick
LIBRARY_DIRECTORY := ../ahocorasick/build
LINK_LIBRARY := ahocorasick
CFLAGS := -Wall
HERADERS := $(wildcard *.h)
OBJS := $(patsubst %.c,%.o,$(wildcard *.c))
OBJSFILES := $(patsubst %.o,$(BUILD_DIRECTORY)/%.o,$(OBJS))

ifeq ($(wildcard $(LIBRARY_DIRECTORY)/lib$(LINK_LIBRARY).a),) 
all:;@echo 'Please go to ../ahocorasick directory and complie it first.'
else
all: $(PROGRAM)
endif

$(PROGRAM): mkbuilddir $(OBJS)
	cc -o $(BUILD_DIRECTORY)/$(PROGRAM) $(OBJSFILES) $(CFLAGS) -L$(LIBRARY_DIRECTORY) -l$(LINK_LIBRARY)

multifast.o: multifast.c $(HERADERS)
	cc -o $(BUILD_DIRECTORY)/multifast.o -c multifast.c $(CFLAGS) -I$(INCLUDE_DIRECTORY)

pattern.o: pattern.c $(HERADERS)
	cc -o $(BUILD_DIRECTORY)/pattern.o -c pattern.c $(CFLAGS) -I$(INCLUDE_DIRECTORY)

reader.o: reader.c $(HERADERS)
	cc -o $(BUILD_DIRECTORY)/reader.o -c reader.c $(CFLAGS) -I$(INCLUDE_DIRECTORY)

strmm.o: strmm.c $(HERADERS)
	cc -o $(BUILD_DIRECTORY)/strmm.o -c strmm.c $(CFLAGS) -I$(INCLUDE_DIRECTORY)

walker.o: walker.c $(HERADERS)
	cc -o $(BUILD_DIRECTORY)/walker.o -c walker.c $(CFLAGS) -I$(INCLUDE_DIRECTORY)

mkbuilddir:
	mkdir -p $(BUILD_DIRECTORY)

clean:
	rm -rf $(BUILD_DIRECTORY)
