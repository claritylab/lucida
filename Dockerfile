####
# Builds the lucida base image
FROM ubuntu:14.04

#### environment variables
ENV LUCIDAROOT /usr/local/lucida/lucida
ENV THRIFT_ROOT /usr/local/lucida/tools/thrift-$THRIFT_VERSION
ENV LD_LIBRARY_PATH /usr/local/lib
ENV CAFFE /usr/local/lucida/tools/caffe/distribute
ENV CPU_ONLY 1 # for caffe

ENV OPENCV_VERSION 2.4.9
ENV THRIFT_VERSION 0.9.3
ENV THREADS 4
ENV PROTOBUF_VERSION 2.5.0
ENV JAVA_VERSION 8
ENV JAVA_TOOL_OPTIONS=-Dfile.encoding=UTF8 

#### common package installations
RUN sed 's/main$/main universe/' -i /etc/apt/sources.list
RUN apt-get update
RUN apt-get install -y zlib1g-dev
RUN apt-get install -y libatlas3-base
RUN apt-get install -y python2.7-dev
RUN apt-get install -y libblas3
RUN apt-get install -y libblas-dev
RUN apt-get install -y liblapack3
RUN apt-get install -y liblapack-dev
RUN apt-get install -y libc6
RUN apt-get install -y software-properties-common
RUN apt-get install -y gfortran
RUN apt-get install -y make
RUN apt-get install -y ant
RUN apt-get install -y gcc
RUN apt-get install -y g++
RUN apt-get install -y wget
RUN apt-get install -y automake
RUN apt-get install -y git
RUN apt-get install -y curl
RUN apt-get install -y libboost-all-dev
RUN apt-get install -y libevent-dev
RUN apt-get install -y libdouble-conversion-dev
RUN apt-get install -y libtool
RUN apt-get install -y liblz4-dev
RUN apt-get install -y liblzma-dev
RUN apt-get install -y binutils-dev
RUN apt-get install -y libjemalloc-dev
RUN apt-get install -y pkg-config
RUN apt-get install -y libtesseract-dev
RUN apt-get install -y libopenblas-dev
RUN apt-get install -y libblas-dev
RUN apt-get install -y libatlas-dev
RUN apt-get install -y libatlas-base-dev
RUN apt-get install -y libiberty-dev
RUN apt-get install -y liblapack-dev
RUN apt-get install -y cmake
RUN apt-get install -y zip
RUN apt-get install -y unzip
RUN apt-get install -y sox
RUN apt-get install -y libsox-dev
RUN apt-get install -y autoconf
RUN apt-get install -y autoconf-archive
RUN apt-get install -y bison
RUN apt-get install -y swig
RUN apt-get install -y python-pip
RUN apt-get install -y subversion
RUN apt-get install -y libssl-dev
RUN apt-get install -y libprotoc-dev
RUN apt-get install -y supervisor
RUN apt-get install -y flac
RUN apt-get install -y gawk
RUN apt-get install -y imagemagick
RUN apt-get install -y libgflags-dev libgoogle-glog-dev liblmdb-dev
RUN apt-get install -y libleveldb-dev libsnappy-dev libhdf5-serial-dev
RUN apt-get install -y bc
RUN apt-get install -y python-numpy
RUN apt-get install -y flex
RUN apt-get install -y libkrb5-dev
RUN apt-get install -y libsasl2-dev
RUN apt-get install -y libnuma-dev

RUN pip install --upgrade distribute
RUN pip install Flask==0.10.1
RUN pip install Flask-Login==0.3.2
RUN pip install Flask-WTF==0.12
RUN pip install Jinja2==2.8
RUN pip install MarkupSafe==0.23
RUN pip install WTForms==2.1
RUN pip install Werkzeug==0.11.4
RUN pip install argparse==1.2.1
RUN pip install backports-abc==0.4
RUN pip install backports.ssl-match-hostname==3.5.0.1
RUN pip install cached-property==1.3.0
RUN pip install certifi==2016.2.28
RUN pip install config==0.3.9
RUN pip install defer==1.0.3
RUN pip install docker-compose==1.7.1
RUN pip install docker-py==1.8.1
RUN pip install dockerpty==0.4.1
RUN pip install docopt==0.6.2
RUN pip install functools32==3.2.3-2
RUN pip install futures==3.0.5
RUN pip install greenlet==0.4.9
RUN pip install html5lib==0.999
RUN pip install httplib2==0.8
RUN pip install ipaddress==1.0.16
RUN pip install itsdangerous==0.24
RUN pip install jsonlib2==1.5.2
RUN pip install jsonschema==2.5.1
RUN pip install oauthlib==0.6.1
RUN pip install piston-mini-client==0.7.5
RUN pip install pyOpenSSL==0.13
RUN pip install pycrypto==2.6.1
RUN pip install pymongo==3.2.2
RUN pip install pyserial==2.6
RUN pip install pysha3==0.3
RUN pip install python-dateutil==2.5.3
RUN pip install pytz==2016.4
RUN pip install pyxdg==0.25
RUN pip install rauth==0.7.2
RUN pip install reportlab==3.0
RUN pip install requests==2.7.0
RUN pip install scipy==0.17.1
RUN pip install singledispatch==3.4.0.3
RUN pip install six==1.5.2
RUN pip install supervisor==3.0b2
RUN pip install texttable==0.8.4
RUN pip install thrift==0.9.3
RUN pip install tornado==4.3
RUN pip install torthrift==0.0.9
RUN pip install urllib3==1.7.1
RUN pip install virtualenv==15.0.1
RUN pip install websocket-client==0.37.0
RUN pip install scikit-learn==0.17.1
RUN pip install numpy==1.8.2
RUN pip install pandas==0.18.1

#### package specific routines
RUN \
  echo oracle-java$JAVA_VERSION-installer shared/accepted-oracle-license-v1-1 select true | debconf-set-selections && \
  add-apt-repository -y ppa:webupd8team/java && \
  apt-get update && \
  apt-get install -y oracle-java$JAVA_VERSION-installer && \
  rm -rf /var/lib/apt/lists/* && \
  rm -rf /var/cache/oracle-jdk$JAVA_VERSION-installer

## install lucida
# fixes some weird OE compiliation issue
RUN mkdir -p /usr/local/lucida
WORKDIR /usr/local/lucida
ADD . /usr/local/lucida
RUN /usr/bin/make local
