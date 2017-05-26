#!/bin/bash

mkdir -p deps/downloads
cd deps/downloads
uname -a | grep "x86_64" > /dev/null
if [ $? -eq 0 ]; then
  wget -c "https://bin.equinox.io/c/4VmDzA7iaHb/ngrok-stable-linux-amd64.zip" \
  && unzip -o ngrok-stable-linux-amd64.zip \
  && sudo killall ngrok || : \
  && sudo cp ngrok /usr/bin/ngrok
  if [ $? -ne 0 ]; then exit 1; fi
else
  wget -c "https://bin.equinox.io/c/4VmDzA7iaHb/ngrok-stable-linux-386.zip" \
  && unzip ngrok-stable-linux-386.zip \
  && sudo killall ngrok || : \
  && sudo cp ngrok /usr/bin/ngrok
  if [ $? -ne 0 ]; then exit 1; fi
fi
