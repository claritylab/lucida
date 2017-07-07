SUBDIRS=server test
include ../../../Makefile.common

start_server:
	@if [ "$(port)" != "" ]; then \
		cd server; \
		./imm_server $(port); \
	fi

start_test: 
	@if [ "$(port)" != "" ]; then \
		cd test; \
		./imm_test $(port); \
	fi

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
