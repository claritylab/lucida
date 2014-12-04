#!/bin/bash

IFS=$'\n'; 
for line in `cat sirius_questions_new.txt`;
do
    echo "(1) Your voice search (text) is:"
    echo "$line"      

    filename=${line// /.}
    filename=`echo $filename | tr '[:upper:]' '[:lower:]'`

    echo "(2) Sending request to server..."
    resp=`wget -q -U "Mozilla/5.0" --post-file ./wav/questions/$filename.wav --header "Content-Type: audio/vnd.wave; rate=16000" -O - "http://localhost:8080/" `
    
    echo "Resp: "$resp | cut -d: -f2

    echo "***********************************************"
done

