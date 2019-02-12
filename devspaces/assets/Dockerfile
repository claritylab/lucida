FROM ubuntu:14.04

LABEL description="Devspaces base implementation for Lucida"

USER root

# install deps
RUN apt-get update && apt-get install -y software-properties-common && \
    add-apt-repository -y ppa:george-edison55/cmake-3.x && \
    add-apt-repository -y ppa:pi-rho/dev && \
    curl -sL https://deb.nodesource.com/setup_10.x | sudo -E bash - && \
    apt-get update && \
    apt-get install -y make cmake curl git nodejs python-software-properties software-properties-common tmux=2.0-1~ppa1~t && \
    apt-get clean

ENV JAVA_TOOL_OPTIONS=-Dfile.encoding=UTF8

COPY . /app/lucida/tools/

# the following layers are separate as they install different components
WORKDIR /app/lucida/tools
RUN /bin/bash apt_deps.sh
RUN /bin/bash install_python.sh
RUN /bin/bash install_java.sh
RUN /bin/bash install_opencv.sh
RUN /bin/bash install_thrift.sh
RUN /bin/bash install_fbthrift.sh
RUN /bin/bash install_mongodb.sh
RUN rm -rf /app/lucida

WORKDIR /data


EXPOSE 3000
EXPOSE 27017

# Set default container command
ENTRYPOINT /usr/bin/mongod --config /etc/mongod.conf
