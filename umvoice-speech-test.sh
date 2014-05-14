#!/bin/bash


count=1

IFS=$'\n'; 
for line in `cat umvoice_questions_new_1q.txt`; 
#for line in `cat umvoice_questions_new_2q.txt`; 
do
#while read line; 
#do 
        echo "(1) Your voice search (text) is:"
        echo "$line"      
        
        filename=${line// /.}
        filename=`echo $filename | tr '[:upper:]' '[:lower:]'`

#        play $filename.wav 1> /dev/null 2>/dev/null      

        echo "(2) Sending request to server..."
        wget -q -U "Mozilla/5.0" --post-file ./wav/$filename.wav --header "Content-Type: audio/vnd.wave; rate=16000" -O - "http://localhost:8080/" 
                
        echo "***********************************************"

#done < siri_questions.txt
done

