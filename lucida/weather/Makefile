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
	rm -rf lucidaservice lucidatypes

start_server:
	cd server; python WeatherServer.py

start_test:
	cd client; python WeatherClient.py
