start_client: 
	gnome-terminal -x bash -c ". venv/bin/activate; python FakeCMDClient.py $(filter-out $@,$(MAKECMDGOALS)); read -n1"

start_server: 
	gnome-terminal -x bash -c ". venv/bin/activate; python app.py $(filter-out $@,$(MAKECMDGOALS)); read -n1"

.PHONY:	all venv thrift clean start_client start_server
