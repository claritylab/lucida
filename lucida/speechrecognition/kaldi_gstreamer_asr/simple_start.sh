export GST_PLUGIN_PATH=$(pwd)/kaldi/tools/gst-kaldi-nnet2-online/src

gnome-terminal -x bash -c "python kaldigstserver/worker.py -u ws://localhost:8081/worker/ws/speech -c sample_english_nnet2.yaml; read -n1"
