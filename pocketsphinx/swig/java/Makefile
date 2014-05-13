CFLAGS := -g -Wall -DPIC -fPIC
CPPFLAGS := -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux `pkg-config --cflags sphinxbase pocketsphinx`

all: libpocketsphinx_jni.so

libpocketsphinx_jni.so: pocketsphinx_wrap.o sphinxbase_wrap.o
	$(CC) -shared -o $@ pocketsphinx_wrap.o sphinxbase_wrap.o `pkg-config --libs sphinxbase pocketsphinx`

pocketsphinx_wrap.c: ../pocketsphinx.i
	mkdir -p edu/cmu/pocketsphinx
	swig -I.. -I../../../sphinxbase/swig -I../include -I../../sphinxbase/include \
		-java -package edu.cmu.pocketsphinx \
		-outdir edu/cmu/pocketsphinx -o $@ $<

sphinxbase_wrap.c: ../../../sphinxbase/swig/sphinxbase.i
	mkdir -p edu/cmu/pocketsphinx
	swig -I.. -I../../../sphinxbase/swig -I../include -I../../sphinxbase/include \
		-java -package edu.cmu.pocketsphinx \
		-outdir edu/cmu/pocketsphinx -o $@ $<

clean:
	$(RM) *.so *_wrap.c *_wrap.o test/*.class
	$(RM) -r edu

run:
	javac -cp junit-4.8.1.jar test/*.java edu/cmu/pocketsphinx/*.java
	java -Djava.library.path=. -cp junit-4.8.1.jar:. org.junit.runner.JUnitCore test.DecoderTest

