#!/bin/bash

QA="http://141.212.106.240:8080"
ASR="http://141.212.106.240:8081"

read -p "Press [Enter] key to start speaking..."
 
echo "Recording... Press Ctrl+C to Stop."
sox -d speech.wav 1> /dev/null 2> /dev/null
# echo "(1) Converting your audio to suitable format for speech processing..."
ffmpeg -y -i speech.wav -acodec pcm_s16le -ac 1 -ar 16000 speech2.wav 1>/dev/null 2>/dev/null

# echo "(2) Just to make sure, you said..."
# play speech2.wav 1> /dev/null 2>/dev/null

# echo "(3) OK, now sending the recorded speech to server..."

resp=`wget -q -U "Mozilla/5.0" --post-file speech2.wav --header "Content-Type: audio/vnd.wave; rate=16000" -O - $ASR`

echo "Speech-to-text:" $resp

query=`echo $resp | cut -d: -f2`
query="$query"

echo "Query: " $query
say "Asking sirius..."
say $query

query2=$(echo -n $query | perl -pe's/([^-_.~A-Za-z0-9])/sprintf("%%%02X", ord($1))/seg');

resp2=`curl --request GET "$QA?query=$query2" 2>/dev/null`

echo "QA response:" $resp2

ans="your answer is:"
say $ans
say $resp2
