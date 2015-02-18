#!/usr/bin/env bash

wav_file=$1
dest_file=$2

echo "Convert sampling rate of the audio file to 8000"
echo "Source:$wav_file, Dest:$dest_file"

sox $wav_file -G -r 8000 $dest_file 
