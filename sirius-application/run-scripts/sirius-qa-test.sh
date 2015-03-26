#!/usr/bin/env bash

[[ -n "$QA" ]] || QA="http://localhost:8080"
query=$1

echo "(1) Your query text is:"
echo "$query"

query=$(echo -n $query | perl -pe's/([^-_.~A-Za-z0-9])/sprintf("%%%02X", ord($1))/seg');

echo "(2) Sending request to server..."
curl --request GET "$QA?query=$query"

echo "***********************************************"
