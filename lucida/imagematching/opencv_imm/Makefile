SUBDIRS=server test
include ../../../Makefile.common

start_server: 
	cd server; ./imm_server

start_test: 
	cd test; ./imm_test 

docker:
	cp ../../lucidaservice.thrift .; \
	cp ../../lucidatypes.thrift .; \
	cp ../../../Makefile.common .; \
	docker build -t lucida_imm .; \
	rm lucidaservice.thrift; \
	rm lucidatypes.thrift; \
	rm Makefile.common;

%:
	@:
