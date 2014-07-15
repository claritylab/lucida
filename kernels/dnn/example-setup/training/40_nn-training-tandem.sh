#!/bin/bash
#set -x

NNTRAINER=./executables/nn-trainer.*
CONFIG=config/nn-training.config
ALIGN=data/am.lda-2.alignment.7.cache
CACHE=data/mfcc.features.cache
CART=data/cart.2.tree
NORM_MEAN=data/mean-f32.xml
NORM_STD=data/std-f32.xml
LEARNING_RATES=(0.5 0.5 0.5 0.5 0.5 0.25 0.25 0.1 0.05 0.01)
EPOCHS=${#LEARNING_RATES[*]}
#export OMP_NUM_THREADS=4

# split training data in training/evaluation parts
function shuffle_segments() {
    if [ ! -e data/segments.all ]; then
        executables/archiver.* \
            --mode list $CACHE | \
            grep AN4 | grep -v attribs | awk '{print $1}' | sort > data/segments.all
    fi

    tmp=$(mktemp)
    sort -R data/segments.all > $tmp
    head -n 90  $tmp | sort > data/segments.cv
    tail -n 858 $tmp | sort > data/segments.train
}

# extract statistics from log file
function statistics_tr() {
    local log=$1
    fer=$(grep frame-classification-error-rate-on-batch $log | \
            tr '<>' ' ' | \
            awk '{print $2}' | \
            awk 'BEGIN{X=0;N=0}{X+=$1;N+=1}END{print X/N}')
    echo $fer
}

function statistics_cv() {
    local log=$1
    fer=$(grep total-frame-classification-error $log | \
            awk '{print $2}')
    echo $fer
}

# perform a training epoch
function train_epoch() {
    local learning_rate=$1
    local model_old=$2
    local model_new=$3
    local log=$4
    #local segments=data/segments.all
    local segments=data/segments.train

    $NNTRAINER \
        --config=$CONFIG \
        --*.network-type=bottleneck \
        --*.base-feature-extraction-cache.path=$CACHE \
        --*.alignment-cache.path=$ALIGN \
        --*.state-tying.file=$CART \
        --*.mean-file=$NORM_MEAN \
        --*.standard-deviation-file=$NORM_STD \
        \
        --*.corpus.segments.file=$segments \
        --*.learning-rate=$learning_rate \
        --*.parameters-old=$model_old \
        --*.parameters-new=$model_new \
        --*.channels.output-channel.file=$log
}

# forward and classify eval data
function forward_cv() {
    local model=$1
    local log=$2
    local segments=data/segments.cv

    $NNTRAINER \
        --config=$CONFIG \
        --*.network-type=bottleneck \
        --*.trainer=frame-classification-error \
        --*.shuffle=false \
        --*.base-feature-extraction-cache.path=$CACHE \
        --*.alignment-cache.path=$ALIGN \
        --*.state-tying.file=$CART \
        --*.mean-file=$NORM_MEAN \
        --*.standard-deviation-file=$NORM_STD \
        \
        --*.corpus.segments.file=$segments \
        --*.parameters-old=$model \
        --*.channels.output-channel.file=$log
}

# main training routine
for iter in $(seq $EPOCHS); do
    if [ $iter == 1 ]; then
        model_old=""
    else
        model_old="bin:data/weights-bn-$(($iter-1))"
    fi
    model_new="bin:data/weights-bn-$iter"
    log_tr=log/nn-training-bn.$iter.log
    log_cv=log/nn-training-bn.cv.$iter.log
    learning_rate=${LEARNING_RATES[$(($iter-1))]}

    echo "==========================================================="
    echo "Starting epoch $iter with learning rate $learning_rate"
    date
    shuffle_segments
    train_epoch $learning_rate "$model_old" "$model_new" "$log_tr"
    forward_cv  "$model_new" "$log_cv"
    fer_tr=$(statistics_tr $log_tr)
    fer_cv=$(statistics_cv $log_cv)
    echo "Frame classification error rate on train/cv in epoch $iter: $fer_tr / $fer_cv"
done
