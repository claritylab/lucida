DOCKER_CONTAINER=claritylab/lucida
VERSION=latest

THREADS=4

include ./Makefile.common

.PHONY: docker local

## build docker environment
docker:
	docker build -t $(DOCKER_CONTAINER):$(VERSION) .

## build local environment
export LD_LIBRARY_PATH=/usr/local/lib
export LUCIDAROOT=$(shell pwd)/lucida

local: 
	cd tools && make && cd ../lucida && make

tools:
	cd tools && make

services: tools
	cd lucida && make

## start all services
start_all:
	cd lucida/commandcenter && make start_server && sleep 5 && cd ../speechrecognition/kaldi_gstreamer_asr && make start_server && cd ../../imagematching/opencv_imm && make start_server && cd ../../questionanswering/OpenEphyra && make start_server && cd ../../calendar && make start_server

start_all_except_CMD:
	cd lucida/speechrecognition/kaldi_gstreamer_asr && make start_server && cd ../../imagematching/opencv_imm && make start_server && cd ../../questionanswering/OpenEphyra && make start_server && cd ../../calendar && make start_server

