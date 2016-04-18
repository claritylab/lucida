#!/bin/bash

set -ex

cd $LUCIDAROOT/questionanswering/lucida
./start-qa.sh $DOCKER_QUESTION_ANSWER $DOCKER_COMMAND_CENTER
