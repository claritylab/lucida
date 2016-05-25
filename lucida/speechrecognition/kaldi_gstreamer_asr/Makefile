all:
	@if [ ! -d "kaldi" ]; then \
		git clone https://github.com/kaldi-asr/kaldi.git; \
		cd kaldi; \
		git checkout 526fcad0a8a739be27687097b29dc9055d03db29; \
		cd tools; \
		extras/check_dependencies.sh; \
		make; \
		cd ..; \
		cd src; \
		sed -i '7s/^/COMPILE_FLAGS += -fPIC\n/' Makefile; \
		./configure --shared; \
		make depend; \
		make; \
		make ext; \
		cd gst-plugin; \
		make depend; \
		make; \
		cd ../../; \
		cd tools; \
		git clone https://github.com/alumae/gst-kaldi-nnet2-online.git; \
		cd gst-kaldi-nnet2-online; \
		git checkout 3bd16b03e8659090dbdf5db51f5cf30a20ec3d90; \
		cd src; \
		export KALDI_ROOT=$LUCIDAROOT/speechrecognition/kaldi_gstreamer_asr/kaldi; \
		make depend; \
		make; \
		cd ../../../../; \
		./test/models/download-fisher-nnet2.sh; \
		export GST_PLUGIN_PATH=$LUCIDAROOT/speechrecognition/kaldi_gstreamer_asr/kaldi/tools/gst-kaldi-nnet2-online/src; \
	fi

start_master_server:
	gnome-terminal -x bash -c "python kaldigstserver/master_server.py --port=8888; read -n1"

start_server:
	./simple_start.sh

start_test: 
	gnome-terminal -x bash -c "python kaldigstserver/client.py -r 8192 test/data/bill_gates-TED.mp3; read -n1"

.PHONY:	all start_server start_test
