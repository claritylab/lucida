#!/bin/bash

wget -T 10  http://sourceforge.net/projects/math-atlas/files/Stable/3.8.3/atlas3.8.3.tar.gz || \
  wget --no-check-certificate -T 10 -t 3 http://www.danielpovey.com/files/kaldi/atlas3.8.3.tar.gz

tar xozf atlas3.8.3.tar.gz ATLAS/include
