all: deps

deps:
	sudo pip install -r requirements.txt; \

clean:
	rm -rf *.o venv

start_client: 
	gnome-terminal -x bash -c "python FakeCMDClient.py $(filter-out $@,$(MAKECMDGOALS)); read -n1"

start_server: 
	gnome-terminal -x bash -c "python app.py $(filter-out $@,$(MAKECMDGOALS)); read -n1"

.PHONY:	all venv thrift clean start_client start_server
