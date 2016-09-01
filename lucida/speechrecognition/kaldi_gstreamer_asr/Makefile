all:
	@if [ ! -d "kaldi" ]; then \
		./install_kaldi.sh; \
	fi

start_master_server:
	gnome-terminal -x bash -c "source ${PWD}/../../../tools/python_2_7_9/bin/activate && python kaldigstserver/master_server.py --port=8081; read -n1"

start_server:
	./simple_start.sh

start_test: 
	gnome-terminal -x bash -c "source ${PWD}/../../../tools/python_2_7_9/bin/activate && python kaldigstserver/client.py -r 8192 test/data/bill_gates-TED.mp3; read -n1"

.PHONY:	all start_server start_test
