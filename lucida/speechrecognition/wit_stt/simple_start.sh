#!/bin/bash
source $(pwd)/../../config.properties
if [ -z $SECURE_HOST ]; then
	export ASR_ADDR_PORT="ws://localhost:$CMD_PORT"
else
	export ASR_ADDR_PORT="wss://localhost:$CMD_PORT"
fi
#	export ASR_ADDR_PORT="wss://lucida.homelinuxserver.org:443"
python worker.py -u ${ASR_ADDR_PORT}/worker/ws/speech -c wit.yaml
