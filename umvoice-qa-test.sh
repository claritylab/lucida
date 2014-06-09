#!/bin/bash


count=1

IFS=$'\n'; 
for line in `cat umvoice_questions_new_1q.txt`;
#for line in `cat umvoice_questions_new_2q.txt`; 
do
#while read line; 
#do 
        echo "(1) Your query text is:"
        echo "$line"      
        
        query=$(echo -n $line | perl -pe's/([^-_.~A-Za-z0-9])/sprintf("%%%02X", ord($1))/seg');

        echo "(2) Sending request to server..."
        curl --request GET "http://localhost:8081?query=$query"
             
        echo "***********************************************"

#done < siri_questions.txt
done

