#!/usr/bin/env bash
#WMAT01 - 10/03/2015

intip=10.240.0.3
dport=80

sirdir=$HOME/sirius
rundir=$sirdir/sirius-application/run-scripts

cd $rundir
echo "Starting QA Server...."
./start-qa-server.sh localhost 8081 &

echo "Starting ASR Server..."
./start-asr-server.sh pocketsphinx localhost 8082 &

echo "Starting IMM Server..."
./start-imm-server.sh localhost 8083 &

sleep 20 && \

echo "Starting Web Server..."
cd ../../sirius-web
./web-sirius.py $intip $dport & 

echo "Web Server should be running on $intip:$dport..."

exit 0
