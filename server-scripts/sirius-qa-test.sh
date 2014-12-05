#!/bin/bash


IFS=$'\n'; 
for line in `cat sirius-questions.txt`;
do
        echo "(1) Your query text is:"
        echo "$line"      
        
        query=$(echo -n $line | perl -pe's/([^-_.~A-Za-z0-9])/sprintf("%%%02X", ord($1))/seg');

        echo "(2) Sending request to server..."
        time curl --request GET "http://localhost:8080?query=$query"
             
        echo "***********************************************"
        exit
done

