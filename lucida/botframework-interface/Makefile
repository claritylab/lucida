all:
	if [ -z `which npm` ]; then sudo apt-get install npm nodejs; fi
	npm install

start_server:
	bash start_interface.sh

.PHONY:	all start_server
