#!/bin/bash

## TODO
# Export indexes, models, opencv paths no absolute stuff
# server end2end
# Configs:
# pocketsphinx/sphinx4 server
# question list
# database
# ASR params

function print_usage {
    echo "End-to-end OpenSiri+Image pipeline"
    echo "  Usage $0 <wav> <image> <matching db>"
    echo -n "  Example $0 wav/images/building.wav vision/matching/buildings/query_small/1.JPG"
    echo    " vision/matching/buildings/db_small"
}

if ( (( $# < 3 )) ); then
	print_usage
	exit
fi

#Exports
export INDRI_INDEX=`pwd`/wiki_indri_index

# Image matching
timgstart=`date +%s%N`
./vision/detect --match $2 --database $3 &
vision_pid=$!

# Voice
tstart=`date +%s%N`
pocketsphinx_continuous \
    -lw   7.0   \
    -topn 16 \
    -fillprob   1e-6 \
    -silprob	0.1 \
    -wip		0.5 \
    -compallsen	yes \
    -beam 1e-80 \
    -maxhmmpf 30000 \
    -infile $1 \
    -lm models/lm_giga_64k_nvp_3gram.lm.DMP \
    -hmm models/voxforge_en_sphinx.cd_cont_5000 \
    -dict models/cmu07a.dic \
    -logfn /dev/null > res.out

tend=`date +%s%N`
ASR=$(awk "BEGIN {printf \"%.2f\",$(( $tend-$tstart ))/1000000000}")

# wait for matching to finish
wait $vision_pid

tend=`date +%s%N`
IMG=$(awk "BEGIN {printf \"%.2f\",$(( $tend-$timgstart ))/1000000000}")
front=$(awk "BEGIN {printf \"%.2f\",$(( $tend-$tstart ))/1000000000}")

# clean ASR result
sed -i -e "s/[0]\{1,\}\://" res.out 
TXT=$(head -n 1 res.out)

# pocket_pid=$!
# wait $pocket_pid

# Question-Answering
cd openephyra/scripts;
tstart=`date +%s%N`
./OpenEphyra.sh $TXT | grep -v "\.\.\.\|Filter"
tend=`date +%s%N`
QA=$(awk "BEGIN {printf \"%.2f\",$(( $tend-$tstart ))/1000000000}")
# total=$(awk "BEGIN {printf \"%.2f\",$(( $front+$back ))}")

# Results
echo ""
echo "ASR, Vision, QA"
echo "$ASR, $IMG, $QA"

cd - >/dev/null; rm -rf res.out;
