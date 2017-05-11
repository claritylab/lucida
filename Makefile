DOCKER_CONTAINER=lucida
VERSION=latest

ifeq ($(THREADS),)
  THREADS=$(shell grep -c ^processor /proc/cpuinfo)
endif

include ./Makefile.common

.PHONY: docker local

## build docker environment
docker:
	docker build -t $(DOCKER_CONTAINER):$(VERSION) .

## build local environment
export LD_LIBRARY_PATH=/usr/local/lib

local:
	cd tools && sudo make && cd ../lucida && make

start_all:
	cd tools && ./start_all_tmux.sh

start_all_secure:
	cd tools && ./start_all_tmux.sh secure

start_test_all:
	cd tools && chmod +x start_test.sh && ./start_test.sh

clean_all_service:
	cd lucida \
	&& cd calendar && make clean \
	&& cd ../commandcenter && make clean \
	&& cd ../djinntonic && make clean \
	&& cd ../imagematching/opencv_imm && make clean \
	&& cd ../../musicservice && make clean \
	&& cd ../questionanswering/OpenEphyra && make clean \
	&& cd ../../weather && make clean \
	&& cd ../..

all_service:
	cd lucida \
	&& cd calendar && make all \
	&& cd ../commandcenter && make all \
	&& cd ../djinntonic && make all \
	&& cd ../imagematching/opencv_imm && make all \
	&& cd ../../musicservice && make all \
	&& cd ../questionanswering/OpenEphyra && make all \
	&& cd ../../weather && make all \
	&& cd ../..