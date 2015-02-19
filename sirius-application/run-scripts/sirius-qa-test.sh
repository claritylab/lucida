#!/bin/bash

QA="http://localhost:8080"

IFS=$'\n'; 
for line in `cat sirius-question.txt`;
do
        echo "(1) Your query text is:"
        echo "$line"      
        
        query=$(echo -n $line | perl -pe's/([^-_.~A-Za-z0-9])/sprintf("%%%02X", ord($1))/seg');

        echo "(2) Sending request to server..."
        curl --request GET "$QA?query=$query"
             
        echo "***********************************************"
done

