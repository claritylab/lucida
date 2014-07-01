#!/bin/bash

## TODO
# Export indexes, models, opencv paths no absolute stuff
# Surpress OE output
# Configs:
# pocketsphinx/sphinx4 server
# question list
# database
# ASR params

function print_usage {
    echo "Runs end-to-end OpenSiri+Image pipeline"
    echo "  Usage $0 <wav> <image>"
}

if ( (( $# < 2 )) ); then
	print_usage
	exit
fi

top=`pwd`

# Image matching
./visual/detect --match $2 --database visual/matching/db &

det_pid=$!

# Voice
pocketsphinx_continuous \
    -lw   7.0   \
    -topn 16 \
    -fillprob   1e-6  \
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

sed -i -e "s/[0]\{1,\}\://" res.out 

TXT=$(head -n 1 res.out)

# pocket_pid=$!
# wait $pocket_pid

wait $det_pid

cd openephyra/scripts;
./OpenEphyra.sh $TXT
rm -rf res.out
