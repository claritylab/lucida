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

SESSION_NAME="lucida"

# Check to see if we should run on http/ws (non-secure) or https/wss (secure)
if [ "$1" == "secure" ]; then
    echo "Enabling secure host"
    # Getting the host IP address
    export ASR_ADDR_PORT="wss://$(/sbin/ifconfig eth0 | grep 'inet addr:' | cut -d: -f2 | awk '{ print $1}'):8081"
    export SECURE_HOST=true

    # Generate self-signed certificates
    cd $(pwd)/../lucida/commandcenter/
    chmod +x gen_certs.sh
    ./gen_certs.sh
    cd $(pwd)/../../tools
else
    echo "Enabling non-secure host"
    export ASR_ADDR_PORT="ws://localhost:8081"
    unset SECURE_HOST
fi

declare -a commandcenter=("CMD" "$(pwd)/../lucida/commandcenter/")
declare -a questionanswering=("QA" "$(pwd)/../lucida/questionanswering/OpenEphyra/")
declare -a imagematching=("IMM" "$(pwd)/../lucida/imagematching/opencv_imm/")
declare -a calendar=("CA" "$(pwd)/../lucida/calendar/")
declare -a speechrecognition=("ASR" "$(pwd)/../lucida/speechrecognition/kaldi_gstreamer_asr/")
declare -a imageclassification=("IMC" "$(pwd)/../lucida/djinntonic/imc/")
declare -a digitrecognition=("DIG" "$(pwd)/../lucida/djinntonic/dig/")
declare -a facerecognition=("FACE" "$(pwd)/../lucida/djinntonic/face")

declare -a services=(
    commandcenter
    questionanswering
    imagematching
    calendar
    speechrecognition
    imageclassification
    digitrecognition
    facerecognition)

# Create the session
tmux new-session -s ${SESSION_NAME} -n vim -d
# First window (0) -- vim
tmux send-keys -t ${SESSION_NAME} 'vim' C-m

# Create the service windows
TMUX_WIN=1
for i in "${services[@]}"
do
    NAME=$i[0]
    SERV_PATH=$i[1]
    tmux new-window -n ${!NAME} -t ${SESSION_NAME}
    tmux send-keys -t ${SESSION_NAME}:$TMUX_WIN "cd ${!SERV_PATH}" C-m
    tmux send-keys -t ${SESSION_NAME}:$TMUX_WIN "make start_server" C-m
    ((TMUX_WIN++))
done

# Start out on the first window when we attach
tmux select-window -t ${SESSION_NAME}:0
