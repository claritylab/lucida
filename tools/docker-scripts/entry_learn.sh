#!/bin/bash

set -ex

cd $LUCIDAROOT/learn
./start-parser-server.sh $DOCKER_LEARN
