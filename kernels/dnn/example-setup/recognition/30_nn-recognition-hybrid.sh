#!/bin/bash
# Recognition using a hybrid NN/HMM model.
# Requires training step 31_nn-training.sh
set -x
ANALOG=analog


export USE_OPENMP=1

export OPENBLAS_NUM_THREADS=8

export OMP_NUM_THREADS=8

#time /home/gpuser/umvoice/kernels/dnn/rwth-asr-0.6.1/arch-gpu/speech-recognizer.* \

time /home/gpuser/umvoice/kernels/dnn/rwth-asr-0.6.1/arch-omp2/speech-recognizer.* \
    --config=config/recognition-hybrid.config \
    --*.base-feature-extraction-cache.path=data/mfcc.features.recognition.cache \
    --*.state-tying.file=data/cart.2.tree \
    --*.mixture-set.file=data/am.lda-2.final-single-gaussian.mix \
    --*.parameters-old=bin:data/weights-10 \
    --*.mean-file=data/mean-f32.xml \
    --*.standard-deviation-file=data/std-f32.xml

#[[ "$?" == 0 ]] && $ANALOG log/recognition-hybrid.log.gz


# MULTIPLE THREAD





# GPU




