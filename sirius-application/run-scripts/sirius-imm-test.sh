#!/usr/bin/env bash

img=$1
[[ -n "$IMM" ]] || IMM="localhost:8082/"

echo "(1) Your image file is:"
echo "$img"      

resp=`wget -q -U "Mozilla/5.0" --post-file $img \
                               --header "Content-Type: image/jpeg" \
                               -O - $IMM `
echo Image data: $resp
