#!/bin/bash

set -ex

cd $LUCIDAROOT/commandcenter
./cmd_server $DOCKER_COMMAND_CENTER
