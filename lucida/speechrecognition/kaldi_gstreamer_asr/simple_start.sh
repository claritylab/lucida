#!/bin/bash
export GST_PLUGIN_PATH=$(pwd)/kaldi/tools/gst-kaldi-nnet2-online/src

python kaldigstserver/worker.py -u ${ASR_ADDR_PORT}/worker/ws/speech -c sample_english_nnet2.yaml
