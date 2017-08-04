.PHONY:	all start_server clean test

thrift:
	@if [ ! -d "lucidaservice" ] || [ ! -d "lucidatypes" ] || [ ! -d "asrthriftservice" ]; then \
		rm -rf lucidaservice lucidatypes; \
		thrift -out ./src/ --gen py ../../lucidaservice.thrift && \
		thrift -out ./src/ --gen py ../../lucidatypes.thrift && \
		thrift -out ./src/ --gen py ../../speechrecognition/thrift/asrthriftservice.thrift || exit 1; \
		rm -rf __init__.py; \
	fi

all: thrift
	sudo apt-get install python3 python3-venv portaudio19-dev libffi-dev libssl-dev
	if [ ! -d env ]; then python3 -m venv env; fi
	env/bin/python -m pip install --upgrade pip setuptools
	. ./env/bin/activate && pip install --upgrade -r requirements.txt && deactivate

test:
	python -m audio_helpers

start_server:
	bash start_server.sh

clean:
	deactivate 2>/dev/null || :
	rm -rf lucidaservice
	rm -rf lucidatypes
	rm -rf *.pyc
	rm -rf config.tmp
	mv config.sh config.bak 2>/dev/null && echo "Backed up old config to config.bak" || :

distclean: clean
	deactivate 2>/dev/null || :
	rm -rf env
