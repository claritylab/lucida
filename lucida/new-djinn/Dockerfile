FROM ubuntu:14.04

ENV THRIFT_VERSION 0.9.3
ENV THREADS 8

RUN mkdir -p /usr/local/new-djinn
ADD . /usr/local/new-djinn
WORKDIR "/usr/local/new-djinn/tools"
RUN /bin/bash get-deps.sh
RUN /bin/bash install-thrift.sh
RUN /bin/bash install-fbthrift.sh

WORKDIR = "/usr/local/new-djinn/kaldiasr"
RUN /bin/bash make_asr.sh 
