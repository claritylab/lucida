SUBDIRS=opencv server test
include ../../../Makefile.common

start_server: 
	gnome-terminal -x bash -c "cd server; ./imm_server $(filter-out $@,$(MAKECMDGOALS)); read -n1"

start_test: 
	gnome-terminal -x bash -c "cd test; ./imm_test $(filter-out $@,$(MAKECMDGOALS)); read -n1"

%:
	@:
