## build from lucida base image
FROM lucida_base

## environment variables
ENV LUCIDAROOT /usr/local/lucida/lucida
ENV LD_LIBRARY_PATH /usr/local/lib

## install QA
RUN mkdir -p /usr/local/lucida/lucida/questionanswering/OpenEphyra
ADD . /usr/local/lucida/lucida/questionanswering/OpenEphyra
WORKDIR "/usr/local/lucida/lucida/questionanswering/OpenEphyra"
RUN mv Makefile.common ../../../ 
RUN mv lucidaservice.thrift ../../ && mv lucidatypes.thrift ../../
RUN /usr/bin/make
