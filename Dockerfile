FROM ubuntu:12.04

MAINTAINER Luke Benson <luke.benson@emergingstack.com>

RUN apt-get update && apt-get install -y \
	python-software-properties

RUN echo 'deb http://archive.ubuntu.com/ubuntu precise multiverse' >> /etc/apt/sources.list

RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test \
	&& add-apt-repository -y ppa:tuleu/precise-backports

RUN apt-get update && apt-get install -y \
	openjdk-7-jdk \
	sox \
	unzip \
	wget \
	git \
	curl \
	make \
	cmake \
	subversion \
	python-dev \
	python-numpy \
	python-dev \
	python-numpy \
	libleveldb-dev \
	libsnappy-dev \
	libboost-dev \
	libboost-system-dev \
	libboost-python-dev \
	libboost-thread-dev \
	libprotobuf-dev \
	libfaac-dev \
	libatlas-base-dev \
	libblas-dev libopenblas-dev \
	libhdf5-serial-dev \
	libgflags-dev \
	libgoogle-glog-dev \
	bc \
	gcc-4.8 \
	g++-4.8 \
	tesseract-ocr \
	tesseract-ocr-eng \
	libtesseract-dev \
	libleptonica-dev \
	libatlas-dev \
	ant \
	automake \
	autoconf \
	libtool \
	bison \
	libboost-all-dev \
	ffmpeg \
	swig \
	build-essential \
	checkinstall \
	libjack-jackd2-dev \
	libmp3lame-dev \
	libopencore-amrnb-dev \
	libopencore-amrwb-dev \
	libsdl1.2-dev \
	libtheora-dev \
	libva-dev \
	libvdpau-dev \
	libvorbis-dev \
	libx11-dev \
	libxfixes-dev \
	libxvidcore-dev \
	texi2html \
	yasm \
	zlib1g-dev

RUN curl -# -O https://protobuf.googlecode.com/files/protobuf-2.5.0.tar.gz \
    && tar -xzvf protobuf-2.5.0.tar.gz \
    && cd protobuf-2.5.0 \
    && ./configure --prefix=/usr \
    && make \
    && make install

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 50 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 50 \
    && update-java-alternatives -s /usr/lib/jvm/java-1.7.0-openjdk-amd64

CMD "bash"