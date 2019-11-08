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
	@if [ "$(port)" != "" ]; then \
		cd server; \
		python WeatherServer.py $(port); \
	fi

start_test: 
	@if [ "$(port)" != "" ]; then \
		cd client; \
		python WeatherClient.py $(port); \
	fi