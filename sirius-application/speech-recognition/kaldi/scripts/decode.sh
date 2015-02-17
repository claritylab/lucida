#!/usr/bin/env bash

# Online decoding use model pre-trained on fisher_english recipe
# Yiping Kang 2014
# ypkang@umich.edu

# Change this to 
KALDI_DIR=../

WAV_FILE=$1

$KALDI_DIR/src/online2bin/online2-wav-nnet2-latgen-faster \
  --do-endpointing=false --online=false \
  --config=nnet_a_gpu_online/conf/online_nnet2_decoding.conf \
  --max-active=7000 --beam=15.0 --lattice-beam=6.0 \
  --acoustic-scale=0.1 --word-symbol-table=graph/words.txt \
  nnet_a_gpu_online/smbr_epoch2.mdl graph/HCLG.fst \
  "ark:echo utterance-id1 utterance-id1|" "scp:echo utterance-id1 $WAV_FILE|" \
  ark:/dev/null
