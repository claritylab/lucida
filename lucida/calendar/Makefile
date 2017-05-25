all: thrift corenlp client
	./gradlew build

thrift:
	@if [ ! -d "src/main/java/thrift" ]; then \
		thrift --gen java ../lucidaservice.thrift; \
		thrift --gen java ../lucidatypes.thrift; \
		sed -i '1s/^/package thrift;\n/' gen-java/LucidaService.java; \
		sed -i '1s/^/package thrift;\n/' gen-java/QueryInput.java; \
		sed -i '1s/^/package thrift;\n/' gen-java/QuerySpec.java; \
		mkdir src/main/java/thrift/; \
		mv gen-java/* src/main/java/thrift/; \
		rmdir gen-java/; \
    fi

client:
	cd CalendarClient/ && ./compile-Calendar-client.sh

corenlp:
	@if [ ! -d "lib" ]; then \
		wget -c http://nlp.stanford.edu/software/stanford-corenlp-full-2016-10-31.zip; \
		unzip stanford-corenlp-full-2016-10-31.zip; \
		mv stanford-corenlp-full-2016-10-31 lib; \
	fi

start_server: 
	./gradlew run

start_test: 
	cd CalendarClient; ./start-Calendar-client.sh 

clean:
	rm -rf src/main/java/thrift && rm -rf CalendarClient/thrift && rm -rf CalendarClient/*.class && rm -rf build && rm -rf lib

.PHONY:	all thrift gradle start_server start_test clean

%:
	@:
