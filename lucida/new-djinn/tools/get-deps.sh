# Run as superuser

apt-get update
ln -s -f bash /bin/sh
ln -s /usr/bin/pip2 /usr/local/bin/pip

apt-get install -y  g++ \
                    automake \
                    autoconf \
                    autoconf-archive \
                    libtool \
                    libboost-all-dev \
                    libevent-dev \
                    libdouble-conversion-dev \
                    libgoogle-glog-dev \
                    libgflags-dev \
                    liblz4-dev \
                    liblzma-dev \
                    libsnappy-dev \
                    make \
                    zlib1g-dev \
                    binutils-dev \
                    libjemalloc-dev \
                    libssl-dev

# more dependencies
apt-get install -y  libiberty-dev \
                    flex \
                    bison \
                    libkrb5-dev \
                    libsasl2-dev \
                    libnuma-dev \
                    pkg-config \
                    git \
                    wget \
                    python-pip \
                    python-all \
                    python-all-dev \
                    python-all-dbg \
                    cmake \
                    libatlas-base-dev
