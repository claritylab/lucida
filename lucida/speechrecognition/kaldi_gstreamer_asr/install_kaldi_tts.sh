#!/bin/bash
LUCIDAROOT=$(pwd)/../../
git clone https://github.com/bpotard/idlak.git
mv idlak kaldi_tts
cd kaldi_tts
git checkout 02b24dc6f79b84779e423bfbb17bdf8e70c95aec
cd tools
extras/check_dependencies.sh
cp ../../install_idlak.sh . # contains modifications
make
cd ..
cd src
sed -i '7s/^/COMPILE_FLAGS += -fPIC\n/' Makefile
./configure --shared
make depend
make
cd ..
cd egs/tts_dnn_arctic/s1
./run.sh
# Replace the following text with your own.
echo '########## Test: This is a test from Lucida.'
echo 'This is a test from Lucida.' | utils/synthesis_test.sh
echo 'Please "cd exp_dnn/tts_dnn_train_3_deltasc2_quin5/tst_forward_tmp/wav_mlpg" to retrieve the audio!'
