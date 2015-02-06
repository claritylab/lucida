#!/bin/bash

IFS=$'\n'; 
for wav_file in /home/ypkang/input-log/*.wav;
do
    echo "(1) Your voice search (text) is:"
    echo "$line"      

    filename=${line// /.}
    filename=`echo $filename | tr '[:upper:]' '[:lower:]'`

    echo $wav_file
    echo "(2) Sending request to server..."
    resp=`wget -q -U "Mozilla/5.0" --post-file $wav_file \
                                    --header "Content-Type: audio/vnd.wave; rate=16000" \
                                    -O - "localhost:8081/" `
    
    echo "Resp: "$resp | cut -d: -f2

    echo "***********************************************"
done
