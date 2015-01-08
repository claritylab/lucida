#!/bin/bash

cd imm;

ip=localhost
port=8082

if [[ -n "$1" ]]; then
	ip=$1
fi
if [[ -n "$2" ]]; then
	port=$2
fi

./start-img-server.py $ip $port
