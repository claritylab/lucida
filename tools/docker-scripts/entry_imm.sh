#!/bin/bash

set -ex

cd $LUCIDAROOT/imagematching/lucida
./imm_server $DOCKER_IMAGE_MATCHING $DOCKER_COMMAND_CENTER
