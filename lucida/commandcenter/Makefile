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
	gnome-terminal -x bash -c "python app.py $(filter-out $@,$(MAKECMDGOALS)); read -n1"

.PHONY:	all venv thrift clean start_server
