all:
	ngrok version 2>/dev/null | grep -Pe "ngrok version 2\." 2>&1 1>/dev/null || bash deps/install_ngrok.sh
	phantomjs -v 2>/dev/null  | grep -Pe "^2\." 2>&1 1>/dev/null || bash deps/install_phantomjs.sh
	which npm || sudo apt-get install npm nodejs
	npm install

start_server:
	bash start_interface.sh

.PHONY:	all start_server
