#!/usr/bin/env bash

wav_file=$1

[[ -n "$ASR" ]] || ASR="localhost:8081/"

echo "Your audio file is:"
echo $wav_file
echo "Sending request to server $ASR ..."
resp=`wget -q -U "Mozilla/5.0" --post-file $wav_file \
    --header "Content-Type: audio/vnd.wave; rate=16000" \
    -O - $ASR `

echo "Resp: "$resp | cut -d: -f2

echo "***********************************************"
