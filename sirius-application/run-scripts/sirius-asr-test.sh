#!/bin/bash

wav_file=$1

ASR="localhost:8081/"

echo "Your audio file is:"
echo $wav_file
echo "Sending request to server..."
resp=`wget -q -U "Mozilla/5.0" --post-file $wav_file \
    --header "Content-Type: audio/vnd.wave; rate=16000" \
    -O - $ASR" `

echo "Resp: "$resp | cut -d: -f2

echo "***********************************************"
