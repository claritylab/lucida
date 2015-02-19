#!/bin/bash

ASR="http://141.212.106.240:8081/"
QA="http://141.212.106.240:8080/"

IFS=$'\n'; 
for line in `cat sirius-questions.txt`;
do
    echo "Your voice search (text) is:"
    echo "$line"      

    filename=${line// /.}
    filename=`echo $filename | tr '[:upper:]' '[:lower:]'`

    echo "Sending request to ASR server..."
    resp=`wget -q -U "Mozilla/5.0" --post-file ../inputs/questions/$filename.wav \
                                   --header "Content-Type: audio/vnd.wave; rate=16000" \
                                   -O - "$ASR" `
    
    query=`echo $resp | cut -d: -f2`  
    query2=$(echo -n $query | perl -pe's/([^-_.~A-Za-z0-9])/sprintf("%%%02X", ord($1))/seg');

    echo "Sending request to QA server..."
    curl --request GET "$QA?query=$query2"

    echo "***********************************************"
done

