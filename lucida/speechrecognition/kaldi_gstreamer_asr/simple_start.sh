#gnome-terminal -x bash -c "python kaldigstserver/master_server.py --port=8888; read -n1"

export GST_PLUGIN_PATH=kaldi/tools/gst-kaldi-nnet2-online/src/:kaldi/src/gst-plugin/

gnome-terminal -x bash -c "python kaldigstserver/worker.py -u ws://localhost:8888/worker/ws/speech -c sample_english_nnet2.yaml; read -n1"
