#!/bin/bash

ip=localhost
port=8080
data=stemmer.csv

pkill java
rm -f $data log.txt
echo "input,time_ms,words" >> $data

IFS=$'\n'; 
for line in `cat prof-questions.txt`;
do
    ./start-qa-server.sh $ip $port &
    sleep 20

    echo "(1) Your query text is:"
    echo "$line"      

    query=$(echo -n $line | perl -pe's/([^-_.~A-Za-z0-9])/sprintf("%%%02X", ord($1))/seg');

    echo "(2) Sending request to server..."
    time curl --request GET "http://$ip:$port?query=$query"

    echo "***********************************************"

    # rm all until this line
    sed -i '1,/Normalization/d' log.txt
    echo -n "$line," >> $data
    total=$(cat log.txt | grep "stemmer_ns" | awk -F "," '{print $2}' | awk '{total+=$0} END {printf total/1000000}')
    echo -n "$total," >> $data
    cat log.txt | grep "stemmer_ns" | wc -l >> $data
    
    pkill java
    rm -f log.txt
done

