## build from lucida base image
FROM lucida_base

## environment variables
ENV LUCIDAROOT /usr/local/lucida/lucida
ENV LD_LIBRARY_PATH /usr/local/lib

## install QA
RUN mkdir -p /usr/local/lucida/lucida/imagematching/opencv_imm
ADD . /usr/local/lucida/lucida/imagematching/opencv_imm
WORKDIR "/usr/local/lucida/lucida/imagematching/opencv_imm"
RUN mv Makefile.common ../../../ 
RUN mv lucidaservice.thrift ../../ && mv lucidatypes.thrift ../../
RUN /usr/bin/make
