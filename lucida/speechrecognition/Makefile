.PHONY:	include plugin setup decoders all start_server

include:
	./gen_include.sh

plugin: include
	cd src/gstplugin && [ -f configure ] || ./autogen.sh && [ -f Makefile ] || ./configure && make

setup:
	./setup.sh

decoders:
	@for decoder in $$(find decoders -maxdepth 1 -mindepth 1 -type d -not -path decoders/templates); do \
		if [ -d $$decoder ]; then \
			cd $$decoder; \
			make all; \
			make setup; \
			cd ../..; \
		fi; \
	done;

all: plugin decoders #setup

start_server:
	GST_PLUGIN_PATH=`pwd`"/src/gstplugin/src" python src/server.py
