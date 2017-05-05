all: thrift
	ant

thrift:
	@if [ ! -d "src/lucida/thrift" ]; then \
       thrift --gen java ../../lucidaservice.thrift; \
       thrift --gen java ../../lucidatypes.thrift; \
       sed -i '1s/^/package lucida.thrift;\n/' gen-java/LucidaService.java; \
       sed -i '1s/^/package lucida.thrift;\n/' gen-java/QueryInput.java; \
       sed -i '1s/^/package lucida.thrift;\n/' gen-java/QuerySpec.java; \
       mkdir src/lucida/thrift/; \
       mv gen-java/* src/lucida/thrift/; \
       rmdir gen-java/; \
    fi

start_server: 
	gnome-terminal -x bash -c "chmod +x start_server.sh && ./start_server.sh $(filter-out $@,$(MAKECMDGOALS)); read -n1"

start_test: 
	gnome-terminal -x bash -c "chmod +x start_test.sh && ./start_test.sh $(filter-out $@,$(MAKECMDGOALS)); read -n1"

clean:
	rm -rf bin
	rm -rf db
	rm -rf src/lucida/thrift

docker:
	cp ../../lucidaservice.thrift .; \
	cp ../../lucidatypes.thrift .; \
	cp ../../../Makefile.common .; \
	docker build -t lucida_qa .; \
	rm lucidaservice.thrift; \
	rm lucidatypes.thrift; \
	rm Makefile.common;

.PHONY:	all thrift start_server start_test clean docker

%:
	@:
