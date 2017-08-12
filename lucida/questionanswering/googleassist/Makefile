.PHONY:	thrift all setup start_server clean distclean

thrift:
	@if [ ! -d "lucidaservice" ] || [ ! -d "lucidatypes" ] || [ ! -d "asrthriftservice" ]; then \
		rm -rf lucidaservice lucidatypes; \
		thrift -out ./src/ --gen py ../../lucidaservice.thrift && \
		thrift -out ./src/ --gen py ../../lucidatypes.thrift || exit 1; \
		rm -rf src/__init__.py; \
	fi

all: thrift
	sudo apt-get install python portaudio19-dev libffi-dev libssl-dev python-pip
	sudo -H pip install --upgrade pip setuptools
	sudo -H pip install --upgrade -r requirements.txt

setup:
	./setup.sh

start_server: setup
	python src/ga_server.py

clean:
	rm -rf lucidaservice
	rm -rf lucidatypes
	rm -rf *.pyc
	rm -rf .config
	mv configuration.py configuration.bak 2>/dev/null && echo "Backed up old config to configuration.bak" || :

distclean: clean
	rm -rf auth
