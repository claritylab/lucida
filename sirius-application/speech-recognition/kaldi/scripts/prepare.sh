#!/usr/bin/env bash

# Download and prepare online decoding
# Yiping Kang
# ypkang@umich.edu 2014

# Download model and other related files from kaldi-asr.org
echo "Dowload model and other related files"
echo "It could take a while for the server to respond..."
wget http://kaldi-asr.org/downloads/build/5/trunk/egs/fisher_english/s5/exp/nnet2_online/nnet_a_gpu_online/archive.tar.gz -O nnet_a_gpu_online.tar.gz

wget http://kaldi-asr.org/downloads/build/2/sandbox/online/egs/fisher_english/s5/exp/tri5a/graph/archive.tar.gz -O graph.tar.gz

echo "Extract files"
mkdir -p nnet_a_gpu_online graph

tar zxvf nnet_a_gpu_online.tar.gz -C nnet_a_gpu_online
tar zxvf graph.tar.gz -C graph

echo "Modify the pathname in the config files"
# Modify the pathname in the config files
for x in nnet_a_gpu_online/conf/*conf; do
  cp $x $x.orig
  sed s:/export/a09/dpovey/kaldi-clean/egs/fisher_english/s5/exp/nnet2_online/:$(pwd)/: < $x.orig > $x
done

echo "Download a single wav file for test purpose"
# Download a single wav file to demo the decoding
wget http://www.signalogic.com/melp/EngSamples/Orig/ENG_M.wav

echo "Preparation done."
