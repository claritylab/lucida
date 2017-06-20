all: thrift client
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
	cd templateClient/ && ./compile-template-client.sh

start_server: 
	@if [ "$(port)" != "" ]; then \
		./gradlew run -Pargs=$(port); \
	fi

start_test: 
	@if [ "$(port)" != "" ]; then \
		cd templateClient; \
		./start-template-client.sh $(port); \
	fi

clean:
	rm -rf src/main/java/thrift ; \
	rm -rf templateClient/thrift ; \
	rm -rf templateClient/*.class ; \
	rm -rf build;

.PHONY:	all thrift gradle start_server start_test clean

%:
	@:
