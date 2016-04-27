SUBDIRS=kaldi server test
include ../../../Makefile.common

start_server: 
	gnome-terminal -x bash -c "cd server; ./asr_server $(filter-out $@,$(MAKECMDGOALS)); read -n1"

start_test: 
	gnome-terminal -x bash -c "cd test; ./asr_client $(filter-out $@,$(MAKECMDGOALS)); read -n1"

%:
	@:
