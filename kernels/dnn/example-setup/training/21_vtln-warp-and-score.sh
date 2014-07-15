#!/bin/bash
# VTLN step 2/3: for every possible warping factor: extract features and calculate
# likelihood in order to select the best warping factor for each segment

AMT=./executables/acoustic-model-trainer.*
CONFIG=config/triphone-lda-warp.config
INITALIGN=data/am.lda-2.alignment.7.cache
INITMIX=data/am.lda-2.single-gaussian.mix
NAME=am.lda-2.warp-and-score
ALPHAS=../signal-analysis/vtln/warping-factors.xml
SEEN_ALPHAS=data/reduced-warping-factors.xml
LDA_ITER=2
LOGDIR=log/warped.mfcc
mkdir -p $LOGDIR
TARGET_MAP=data/warping-factors-map.xml

set -uxe

function warp() {
    local alpha=$1
    local mix=$2
    local align=$3
    local logdir=$4

    LOG=$logdir/score.$alpha.log

    $AMT --config=$CONFIG --LDA_ITER=$LDA_ITER \
         --action=score-features \
         --*.alignment-cache.path=$align \
         --*.warping-factor.default-output=$alpha \
         --*.feature-scorer.acoustic-model.mixture-set.file=$mix \
         --*.feature-scorer.output.channel=$LOG \
         --*.feature-scorer.corpus-key.template="<segment>"
    mv $LOG $logdir/$alpha.score
}

for alpha in $(grep -v '<' $ALPHAS); do
    warp $alpha $INITMIX $INITALIGN $LOGDIR
done

../tools/build-warping-factor-map.py \
        -a $ALPHAS \
        -p $LOGDIR \
        -s .score \
        \
        -m $TARGET_MAP \
        -r $SEEN_ALPHAS

# plot some statistics
GP=$(which gnuplot)
if [ $? == 0 ]; then
    T=$(mktemp)
    PNG=log/gender_distribution.png

    for gender in m f; do
        grep "\-$gender" $TARGET_MAP | grep value | cut -d"'" -f4 | \
        sort | uniq -c | awk '{print $2, $1}' > $T.$gender
    done
    echo "set terminal png; \
          set xlabel 'warping factor'; \
          set ylabel 'absolute count (smoothed)'; \
          plot '$T.m' smooth bezier title 'male', \
               '$T.f' smooth bezier title 'female'" | $GP > $PNG

    echo "see $PNG"
fi
