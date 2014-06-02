TARGET=CRFPP
JAVAC=javac
JAVA=java
JAR=jar
CXX=c++
INCLUDE=/usr/lib/jvm/java-7-openjdk-amd64/include

PACKAGE=org/chasen/crfpp

LIBS=-lcrfpp -lpthread
INC=-I$(INCLUDE) -I$(INCLUDE)/linux

all:
	$(CXX) -O3 -c -fpic $(TARGET)_wrap.cxx  $(INC)
	$(CXX) -shared  $(TARGET)_wrap.o -o lib$(TARGET).so $(LIBS)
	$(JAVAC) $(PACKAGE)/*.java
	$(JAVAC) test.java
	$(JAR) cfv $(TARGET).jar $(PACKAGE)/*.class

test:
	env LD_LIBRARY_PATH=. $(JAVA) test

clean:
	rm -fr *.jar *.o *.so *.class $(PACKAGE)/*.class
	
cleanall:
	rm -fr $(TARGET).java *.cxx
