all:
	cd server ; \
	make all ; \
	cd ../test; \
	make all ; \
	cd ..;

start_server: 
	cd server; ./template_server 

start_test: 
	cd test; ./template_test 

clean:
	cd server ; \
	make clean ; \
	cd ../test; \
	make clean ; \
	cd ..;

docker:
	cp ../lucidaservice.thrift .; \
	cp ../lucidatypes.thrift .; \
	cp ../../Makefile.common .; \
	docker build -t lucida_template .; \
	rm lucidaservice.thrift; \
	rm lucidatypes.thrift; \
	rm Makefile.common;

%:
	@:
