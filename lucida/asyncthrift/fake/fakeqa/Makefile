all: 
	ant

thrift:
	@if [ ! -d "src/lucida/thrift" ]; then \
       thrift --gen java ../../../lucidaservice.thrift; \
       thrift --gen java ../../../lucidatypes.thrift; \
       sed -i '1s/^/package lucida.thrift;\n/' gen-java/LucidaService.java; \
       sed -i '1s/^/package lucida.thrift;\n/' gen-java/QueryInput.java; \
       sed -i '1s/^/package lucida.thrift;\n/' gen-java/QuerySpec.java; \
       mkdir src/lucida/thrift/; \
       mv gen-java/* src/lucida/thrift/; \
       rmdir gen-java/; \
    fi

start_server: 
	gnome-terminal -x bash -c "java -classpath bin:lib/thrift/libthrift-0.9.2.jar:lib/thrift/log4j-1.2.14.jar:lib/thrift/slf4j-api-1.5.8.jar:lib/thrift/slf4j-log4j12-1.5.8.jar lucida.main.QADaemon $(filter-out $@,$(MAKECMDGOALS)); read -n1"

clean:
	rm -rf bin

.PHONY:	all thrift start_server clean

%:
	@:
