#!/bin/bash

# This script is used to start all of the microservices
# for the Lucida project by creating a tmux window for each
# server in the background.
# Example calls:
# $./start_all_tmux.sh secure
# $./start_all_tmux.sh
# To attach to the tmux session
# use the following command:
# $tmux a -t lucida

# source the port properties
. ../lucida/config.properties

if [ "$1" == "test" ]; then
    SESSION_NAME="lu-test"
else
    SESSION_NAME="lucida"
fi

# Check if session already exists
tmux has-session -t ${SESSION_NAME}
if [ $? -eq 0 ]; then
    echo "Session ${SESSION_NAME} already exists."
    exit 0;
elif [ -n "$TMUX" ]; then
    echo "Already in a tmux session"
    exit 0;
else
    echo "Session ${SESSION_NAME} does not exit. Creating a ${SESSION_NAME} session."
fi

# Check to see if we should run on http/ws (non-secure) or https/wss (secure)
if [ "$1" == "secure" ]; then
    echo "Enabling secure host"
    # Getting the host IP address
    export ASR_ADDR_PORT="wss://$(/sbin/ifconfig eth0 | grep 'inet addr:' | cut -d: -f2 | awk '{ print $1}'):$CMD_PORT"
    export SECURE_HOST=true

    # Generate self-signed certificates
    cd $(pwd)/../lucida/commandcenter/
    chmod +x gen_certs.sh
    ./gen_certs.sh
    cd $(pwd)/../../tools
else
    echo "Enabling non-secure host"
    export ASR_ADDR_PORT="ws://localhost:$CMD_PORT"
fi

declare -a commandcenter=("CMD" "$(pwd)/../lucida/commandcenter/")
declare -a questionanswering=("QA" "$(pwd)/../lucida/questionanswering/OpenEphyra/")
declare -a imagematching=("IMM" "$(pwd)/../lucida/imagematching/opencv_imm/")
declare -a calendar=("CA" "$(pwd)/../lucida/calendar/")
declare -a speechrecognition=("ASR" "$(pwd)/../lucida/speechrecognition/kaldi_gstreamer_asr/")
declare -a imageclassification=("IMC" "$(pwd)/../lucida/djinntonic/imc/")
declare -a digitrecognition=("DIG" "$(pwd)/../lucida/djinntonic/dig/")
declare -a facerecognition=("FACE" "$(pwd)/../lucida/djinntonic/face")
declare -a weather=("WE" "$(pwd)/../lucida/weather")
declare -a botframework=("BFI" "$(pwd)/../lucida/botframework-interface")
declare -a musicservice=("MS" "$(pwd)/../lucida/musicservice")

if [ "$1" == "test" ]; then
    declare -a services=(
        )
else
    declare -a services=(
        commandcenter
        speechrecognition)
fi

services+=(
    questionanswering
    imagematching
    calendar
    imageclassification
    digitrecognition
    facerecognition
    weather
    botframework
    musicservice)

# Create the session
tmux new-session -s ${SESSION_NAME} -d

# Create the service windows
TMUX_WIN=0
for i in "${services[@]}"
do
    NAME=$i[0]
    SERV_PATH=$i[1]
    if [ $TMUX_WIN == 0 ]; then
        tmux rename-window -t ${SESSION_NAME}:$TMUX_WIN ${!NAME}
    else
        tmux new-window -n ${!NAME} -t ${SESSION_NAME}
    fi
    tmux send-keys -t ${SESSION_NAME}:$TMUX_WIN "cd ${!SERV_PATH}" C-m
    if [ "$1" == "test" ]; then
        tmux send-keys -t ${SESSION_NAME}:$TMUX_WIN "make start_test" C-m
    else
        tmux send-keys -t ${SESSION_NAME}:$TMUX_WIN "make start_server" C-m
    fi
    ((TMUX_WIN++))
done

# Start out on the first window when we attach
tmux select-window -t ${SESSION_NAME}:0

