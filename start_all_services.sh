gnome-terminal -x bash -c "cd docker-scripts; export DOCKER_COMMAND_CENTER=8090; ./entry_cc.sh; read -n1"

gnome-terminal -x bash -c "cd docker-scripts; export DOCKER_QUESTION_ANSWER=8095; export DOCKER_COMMAND_CENTER=8090; ./entry_qa.sh; read -n1"

gnome-terminal -x bash -c "cd docker-scripts; export DOCKER_IMAGE_MATCHING=8092; export DOCKER_COMMAND_CENTER=8090; ./entry_imm.sh; read -n1"

gnome-terminal -x bash -c "cd docker-scripts; export DOCKER_SPEECH_RECOGNITION=8094; export DOCKER_COMMAND_CENTER=8090; ./entry_asr.sh; read -n1"

cd ..
