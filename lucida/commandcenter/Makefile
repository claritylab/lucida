all: venv thrift 

venv:
	@if [ ! -d "venv" ]; then \
		virtualenv venv --distribute; \
		. venv/bin/activate; \
		pip install -r requirements.txt; \
    fi

thrift:
	@if [ ! -d "lucidaservice" ]; then \
		thrift --gen py:tornado ../lucidaservice.thrift; \
		thrift --gen py:tornado ../lucidatypes.thrift; \
		cd gen-py.tornado; \
		mv * ..; \
		cd ..; \
		rmdir gen-py; \
		rm __init__.py; \
    fi

clean:
	rm -rf *.o gen-py.tornado venv

start_client: 
	gnome-terminal -x bash -c ". venv/bin/activate; python FakeCMDClient.py $(filter-out $@,$(MAKECMDGOALS)); read -n1"

start_server: 
	gnome-terminal -x bash -c ". venv/bin/activate; python app.py $(filter-out $@,$(MAKECMDGOALS)); read -n1"

start_fake_server:
	gnome-terminal -x bash -c ". venv/bin/activate; python PythonServer.py $(filter-out $@,$(MAKECMDGOALS)); read -n1"

start_fake_server:
	gnome-terminal -x bash -c ". venv/bin/activate; python PythonClient.py $(filter-out $@,$(MAKECMDGOALS)); read -n1"

.PHONY:	all venv thrift clean start_client start_server
