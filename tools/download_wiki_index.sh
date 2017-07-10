#!/bin/bash

if [ -d wiki_indri_index ]; then
  echo "Wikipedia index already downloaded, skipping"
  exit 1
else
  echo "Wikipedia index not found: OK"
fi

FREE=`df -k --output=avail "$PWD" | tail -n1`
if [[ $FREE -lt 11523524 ]]; then
     read -p "Not enough space to download. Do you want to continue (Y/n)? " -n 1 -r
     if [[ ! $REPLY =~ ^[Yy]$ ]]
     then
        echo ""
        echo "Aborting."
        exit 1
     fi
    echo ""
else
   echo "Enough space to download.: OK"
fi;

echo "Starting download."

wget -c http://web.eecs.umich.edu/~jahausw/download/wiki_indri_index.tar.gz

if [ $? -ne 0 ]; then
  echo "wget return code.........: FAIL"
  echo "wget exited with nonzero return, please retry"
  exit 1;
else
  echo "wget return code.........: OK"
fi

FILECHECKSUM="322ae59fd0473c6d055ea827ce791c758be1d632e67956ae385eed77aff2f4f3e84db2643e90845b35e1b6204822e9e6"

echo "Download completed. Checking file."
if [ "$(sha384sum wiki_indri_index.tar.gz | cut -d' ' -f1)" == $FILECHECKSUM ]; then
  echo "sha384sum check..........: OK"
  echo "Decompressing data"
  tar xzvf wiki_indri_index.tar.gz
else
  echo "sha384sum check..........: FAIL"
  echo "Broken download:"
  echo "- Delete the file wiki_indri_index.tar.gz"
  echo "- Run this script again"
fi

echo "******************************************************"
echo "Make sure to export wiki_indri_index=$(pwd)/wiki_indri_index"
echo "******************************************************"
