#!/usr/bin/env bash
#WMAT01 - 10/03/2015

# Set currect dir
rsdir=$HOME/sirius/sirius-application/run-script/

# Start... run as sudo
echo "Starting All Sirius (ASR, IMM, QA)..." && \
wait 5 && \

#ASR
echo "Starting ASR ..." 
cd $rsdir
./start-asr-server.h

#IMM $rsdir
echo "Starting IMM ..."
./start-imm-server.h

#ASR
echo "Starting QA ..."
cd $rsdir
./start-qa-server.h


# Final Compile
echo " " && \
echo " " && \
echo "All servers running..." && \
echo " " && \
echo " " && \
