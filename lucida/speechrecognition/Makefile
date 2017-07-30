.PHONY:	all start_server

all:
	cd src/gstplugin && ./autogen.sh && ./configure && make
	python src/worker.py

start_server:
	GST_PLUGIN_PATH=`pwd`"/src/gstplugin/src" python src/worker.py
