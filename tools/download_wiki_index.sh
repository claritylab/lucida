#!/bin/bash

if [ ! -d wiki_indri_index ];
then
  wget http://web.eecs.umich.edu/~jahausw/download/wiki_indri_index.tar.gz
  tar xzvf wiki_indri_index.tar.gz
else
  echo "Wikipedia index already downloaded, skipping"
fi

echo "******************************************************"
echo "Make sure to export wiki_indri_index=`pwd`/wiki_indri_index"
echo "******************************************************"
