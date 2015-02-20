#!/usr/bin/env bash

# Start a kaldi server
# Online decoding use model pre-trained on fisher_english recipe
# Yiping Kang 2014
# ypkang@umich.edu

# Change this to 
KALDI_DIR=../

$KALDI_DIR/src/online2bin/online2-wav-nnet2-latgen-faster \
  --do-endpointing=false --online=true \
  --config=nnet_a_gpu_online/conf/online_nnet2_decoding.conf \
  --max-active=7000 --beam=15.0 --lattice-beam=6.0 \
  --acoustic-scale=0.1 --word-symbol-table=graph/words.txt \
  nnet_a_gpu_online/smbr_epoch2.mdl graph/HCLG.fst \
  "ark:echo utterance-id1 utterance-id1|" "scp:echo utterance-id1 null|" \
  ark:/dev/null
