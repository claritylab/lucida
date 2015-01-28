#!/bin/bash

QA="http://141.212.106.240:8080"
ASR="http://141.212.106.240:8081"
VIS="http://141.212.106.240:8082"

echo "(0) Sending Image to server..."

wget -q -U "Mozilla/5.0" --post-file demo_img.jpg --header "Content-Type: image/jpeg" -O - $VIS

cat resp_img.txt | awk '{print $3}' | awk -F'/' '{print $NF}' | awk -F'.' '{print $1}' | tr _ " " > resp_img_parsed.txt

resp_img=`cat resp_img_parsed.txt`

echo "Image matching (parsed metadata): " $resp_img

rm speech.wav 1> /dev/null 2> /dev/null
rm speech2.wav 1> /dev/null 2> /dev/null

read -p "Press [Enter] key to start speaking..."
 
echo "Recording... Press Ctrl+C to Stop."
#sox -d rec.wav 
#ffmpeg -i rec.wav -f s16le -acodec pcm_s16le rec.raw
#sox -r 16k -e signed-integer -b 16 -c 1 rec.raw rec.wav 
sox -d speech.wav 1> /dev/null 2> /dev/null
echo "(1) Convert your audio to suitable format for speech processing..."
ffmpeg -i speech.wav -acodec pcm_s16le -ac 1 -ar 16000 speech2.wav  1>/dev/null 2>/dev/null
#ffmpeg -i speech.wav -acodec pcm_s16le -ac 1 -ar 48000 speech3.wav 

#rec -e signed-integer -c 1 -r 16000 -b 16 rec.raw 

echo "(2) Just to make sure, you said..."
play speech2.wav 1> /dev/null 2>/dev/null

echo "(3) OK, now sending the recorded speech to server..."
#scp speech2.wav vinicius@ipanema.eecs.umich.edu:~/siri/ 1> /dev/null 2>/dev/null

resp=`wget -q -U "Mozilla/5.0" --post-file speech2.wav --header "Content-Type: audio/vnd.wave; rate=16000" -O - $ASR`

echo "Speech-to-text:" $resp

query=`echo $resp | cut -d: -f2`
query="$query $resp_img"

echo "Query: " $query

query2=$(echo -n $query | perl -pe's/([^-_.~A-Za-z0-9])/sprintf("%%%02X", ord($1))/seg');

#echo "Query: "$query2

resp2=`curl --request GET "$QA?query=$query2" 2>/dev/null`

echo "QA response:" $resp2

say $resp2
