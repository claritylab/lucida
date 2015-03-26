#!/usr/bin/env bash

[[ -n "$QA" ]] || QA="http://localhost:8080/"
[[ -n "$ASR" ]] || ASR="http://localhost:8081/"
wavfile=$1

echo "Your voice search (text) is:"
echo "$wavfile"

echo "Sending request to ASR server..."
resp=`wget -q -U "Mozilla/5.0" --post-file $wavfile \
    --header "Content-Type: audio/vnd.wave; rate=16000" \
    -O - "$ASR" `

query=`echo $resp | cut -d: -f2`  
query2=$(echo -n $query | perl -pe's/([^-_.~A-Za-z0-9])/sprintf("%%%02X", ord($1))/seg');

echo "Sending request to QA server..."
curl --request GET "$QA?query=$query2"

echo "***********************************************"
