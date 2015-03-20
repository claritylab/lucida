#!/usr/bin/env bash
set -e

# Download and prepare online decoding
# Yiping Kang
# ypkang@umich.edu 2014

# Download model and other related files from kaldi-asr.org
echo "[i] Dowload model and other related files"
echo "[i] It could take a while for the server to respond..."
if ( ! [ -f "nnet_a_gpu_online.tar.gz" ] || ( ! [ -z $1 ] &&  [ "$1" == "-f" ] ) ) ; then
  wget http://kaldi-asr.org/downloads/build/5/trunk/egs/fisher_english/s5/exp/nnet2_online/nnet_a_gpu_online/archive.tar.gz -O nnet_a_gpu_online.tar.gz
else
  echo "[i] nnet_a_gpu_online.tar.gz already exists. Use -f to download it again."
  rm -rf nnet_a_gpu_online
fi
if ( ! [ -f "graph.tar.gz" ] || ( ! [ -z $1 ] &&  [ "$1" == "-f" ] ) ) ; then
  wget http://kaldi-asr.org/downloads/build/2/sandbox/online/egs/fisher_english/s5/exp/tri5a/graph/archive.tar.gz -O graph.tar.gz
  rm -rf graph
else
  echo "[i] graph.tar.gz already exists. Use -f to download it again."
fi
echo "[i] Extract files"
mkdir -p nnet_a_gpu_online graph

tar zxvf nnet_a_gpu_online.tar.gz -C nnet_a_gpu_online
tar zxvf graph.tar.gz -C graph

echo "[i] Modify the pathname in the config files"
# Modify the pathname in the config files
for x in nnet_a_gpu_online/conf/*conf; do
  cp $x $x.orig
  sed s:/export/a09/dpovey/kaldi-clean/egs/fisher_english/s5/exp/nnet2_online/:$(pwd)/: < $x.orig > $x
done

echo "[i] Download a single wav file for test purpose"
# Download a single wav file to demo the decoding -> TODO: does not exist
if ( ! [ -f "ENG_M.wav" ] || ( ! [ -z $1 ] &&  [ "$1" == "-f" ] ) ) ; then
  wget -O ENG_M.wav http://www.signalogic.com/melp/EngSamples/Orig/ENG_M.wav
else
  echo "[i] The file ENG_M.wav already exists. Use -f to download it again."
fi

echo "[i] Preparation done."
