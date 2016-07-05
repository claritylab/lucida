SUBDIRS=server test
include ../../../Makefile.common

start_server: 
	gnome-terminal -x bash -c "cd server; ./imm_server $(filter-out $@,$(MAKECMDGOALS)); read -n1"

start_test: 
	gnome-terminal -x bash -c "cd test; ./imm_test $(filter-out $@,$(MAKECMDGOALS)); read -n1"

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
