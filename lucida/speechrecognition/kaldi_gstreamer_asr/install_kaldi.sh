LUCIDAROOT=$(pwd)/../../
git clone https://github.com/kaldi-asr/kaldi.git
cd kaldi
git checkout 526fcad0a8a739be27687097b29dc9055d03db29
cd tools
sudo ln -s -f bash /bin/sh
sudo apt-get install -y zlib1g-dev automake autoconf libtool subversion
sudo apt-get install -y libatlas3-base
extras/check_dependencies.sh
make
cd ..
cd src
sed -i '7s/^/COMPILE_FLAGS += -fPIC\n/' Makefile
./configure --shared
make depend
make
make ext
cd gst-plugin
sudo apt-get install -y libgstreamer1.0-dev
sudo apt-get install -y gstreamer1.0-plugins-good
sudo apt-get install -y gstreamer1.0-plugins-ugly
sudo apt-get install -y gstreamer1.0-tools
make depend
make
cd ../../
cd tools
git clone https://github.com/alumae/gst-kaldi-nnet2-online.git
cd gst-kaldi-nnet2-online
git checkout 3bd16b03e8659090dbdf5db51f5cf30a20ec3d90
sudo add-apt-repository -y ppa:gstreamer-developers/ppa
sudo apt-get -y update
sudo apt-get install -y libjansson-dev
cd src
export KALDI_ROOT=$LUCIDAROOT/speechrecognition/kaldi_gstreamer_asr/kaldi
make depend
make
cd ../../../../
./test/models/download-fisher-nnet2.sh
export GST_PLUGIN_PATH=$LUCIDAROOT/speechrecognition/kaldi_gstreamer_asr/kaldi/tools/gst-kaldi-nnet2-online/src
sudo pip install tornado
sudo apt-get install -y python3.4-dev
sudo apt-get install -y python2.7-dev
sudo apt-get install -y libblas3
sudo apt-get install -y libblas-dev
sudo apt-get install -y liblapack3
sudo apt-get install -y liblapack-dev
sudo apt-get install -y gfortran
sudo apt-get install -y libc6
