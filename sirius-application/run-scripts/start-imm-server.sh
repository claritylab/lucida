#!/usr/bin/env bash

function print_usage {
  echo "Starts the IMM server"
  echo "    Usage $0 <ip> <port>"
  echo "    Default example: $0 localhost 8082"
}

if [ "$1" == "help" ]; then
  print_usage
  exit
fi

ip=localhost
port=8082

if [[ -n "$1" ]]; then
  ip=$1
fi
if [[ -n "$2" ]]; then
  port=$2
fi

# start from top directory
cd ../image-matching ;

./start-imm-server.py $ip $port
