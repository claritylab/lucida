all: venv thrift 

venv:
	@if [ ! -d "venv" ]; then \
		virtualenv venv --distribute; \
		. venv/bin/activate; \
		pip install -r requirements.txt; \
    fi

thrift:
	@if [ ! -d "gen-py" ]; then \
		thrift1 --gen py:twisted ../../../lucidaservice.thrift; \
		thrift1 --gen py:twisted ../../../lucidatypes.thrift; \
		cd gen-py; \
		mv * ..; \
		cd ..; \
		rmdir gen-py; \
    fi

clean:
	rm -rf *.o gen-py lucidaservice lucidatypes __init__.py venv

start_client: 
	gnome-terminal -x bash -c "source venv/bin/activate; python FakeCMDClient.py $(filter-out $@,$(MAKECMDGOALS)); read -n1"

start_server: 
	gnome-terminal -x bash -c "source venv/bin/activate; python FakeCMDServer.py $(filter-out $@,$(MAKECMDGOALS)); read -n1"

.PHONY:	all venv thrift clean start_client start_server
