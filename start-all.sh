#!/usr/bin/env bash
#WMAT01 - 10/03/2015

sirdir=$HOME/sirius
rundir=$sirdir/sirius-application/run-scripts

cd $rundir
echo "Starting QA Server...."
./start-qa-server.sh localhost 8081 &

echo "Starting ASR Server..."
./start-asr-server.sh pocketsphinx localhost 8082 &

echo "Starting IMM Server..."
./start-imm-server.sh localhost 8083 &

# Final Compile
echo " "
echo " "
echo "All IA Servers running..."
echo " "
echo " " 
