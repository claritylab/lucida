all: thrift 

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
	rm -rf *.o gen-py lucidaservice lucidatypes __init__.py

start_server: 
	gnome-terminal -x bash -c "python FakeCMDServer.py $(filter-out $@,$(MAKECMDGOALS)); read -n1"

.PHONY:	all thrift clean start_server
