CXX = g++
CXX_FLAGS = -Wall

LINK_FLAGS = -lopencv_core \
                         -lopencv_highgui \
                         -lopencv_imgproc \
                         -lopencv_nonfree \
                         -lopencv_flann \
                         -lopencv_objdetect \
                         -lopencv_features2d \
                         -lopencv_gpu \
                         -lrt \
                         -lboost_program_options \
                         -lboost_filesystem \
                         -lboost_system \
                         -lprotobuf \
                         -ltesseract \
			 -lthrift

all: gen-cpp/QAService.o 

gen-cpp/QAService.o: gen-cpp/QAService.cpp
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

clean:
	rm -rf *.o gen-* *.class

.PHONY:	all
