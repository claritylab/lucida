#!/bin/bash

mkdir -p deps/downloads
cd deps/downloads
uname -a | grep "x86_64" > /dev/null
if [ $? -eq 0 ]; then
  wget -c "https://bitbucket.org/ariya/phantomjs/downloads/phantomjs-2.1.1-linux-x86_64.tar.bz2" \
  && tar -xf phantomjs-2.1.1-linux-x86_64.tar.bz2 \
  && sudo killall phantomjs || : \
  && sudo cp phantomjs-2.1.1-linux-x86_64/bin/phantomjs /usr/bin/phantomjs
  if [ $? -ne 0 ]; then exit 1; fi
else
  wget -c "https://bitbucket.org/ariya/phantomjs/downloads/phantomjs-2.1.1-linux-i686.tar.bz2" \
  && tar -xf phantomjs-2.1.1-linux-i686.tar.bz2 \
  && sudo killall phantomjs || : \
  && sudo cp phantomjs-2.1.1-linux-i686/bin/phantomjs /usr/bin/phantomjs
  if [ $? -ne 0 ]; then exit 1; fi
fi
