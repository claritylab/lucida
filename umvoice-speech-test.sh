#!/bin/bash

IFS=$'\n'; 
<<<<<<< HEAD
#for line in `cat umvoice_questions_new_1q.txt`;
for line in `cat umvoice_questions_new_2q.txt`; 
=======
for line in `cat umvoice_questions_new.txt`;
>>>>>>> e3f6a2a55387a6af5d90d68b5381a14b64012ed9
do
    echo "(1) Your voice search (text) is:"
    echo "$line"      

    filename=${line// /.}
    filename=`echo $filename | tr '[:upper:]' '[:lower:]'`

    echo "(2) Sending request to server..."
    wget -q -U "Mozilla/5.0" --post-file ./wav/$filename.wav --header "Content-Type: audio/vnd.wave; rate=16000" -O - "http://localhost:8080/" 

    echo "***********************************************"
done

