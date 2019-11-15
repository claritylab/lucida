all: thrift

thrift:
	@if [ ! -d "lucidaservice" ]; then \
        thrift --gen py ../lucidaservice.thrift; \
        thrift --gen py ../lucidatypes.thrift; \
        cd gen-py; \
        mv * ..; \
        cd ..; \
        rmdir gen-py; \
        rm __init__.py; \
    fi

clean:
	rm -rf lucidaservice lucidatypes ; \
	rm -rf *.pyc; 

start_server:
	@if [ "$(port)" != "" ]; then \
		cd server; \
		python server.py $(port); \
	fi

start_test: 
	@if [ "$(port)" != "" ]; then \
		cd client; \
		python client.py $(port); \
	fi