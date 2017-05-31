#!/bin/bash
export GST_PLUGIN_PATH=$(pwd)/kaldi/tools/gst-kaldi-nnet2-online/src
source $(pwd)/../../config.properties
if [ -z $SECURE_HOST ]; then
	export ASR_ADDR_PORT="ws://localhost:$CMD_PORT"
else
	export ASR_ADDR_PORT="wss://localhost:$CMD_PORT"
fi
python kaldigstserver/worker.py -u ${ASR_ADDR_PORT}/worker/ws/speech -c sample_english_nnet2.yaml
