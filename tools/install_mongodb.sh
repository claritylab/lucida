#!/bin/bash
export MONGO_C_DRIVER_VERSION=1.3.0

installCheck () {
  g++ check_mongodb.cpp -std=c++11 -lmongoclient -lboost_thread -lboost_filesystem -lboost_regex -lboost_program_options -lboost_system -pthread -lssl -lcrypto -o check_mongodb
  if [[ $? -ne 0 ]]; then
    return 1
  fi
  if [[ $(./check_mongodb | grep "Connection ok") == "Connection ok" ]]; then
    rm check_mongodb
    return 0
  else
    return 1
  fi
}

if installCheck "$0"; then
  echo "MongoDB and C++ driver installed";
  exit 0;
fi

# MongoDB.
apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv EA312927
echo "deb http://repo.mongodb.org/apt/ubuntu trusty/mongodb-org/3.2 multiverse" | tee /etc/apt/sources.list.d/mongodb-org-3.2.list
apt-get update
apt-get install -y mongodb-org
service mongod start

if [ `ps -ef | grep -cPe "\Wmongod "` -lt 1 ]; then
  cp mongodb.service /lib/systemd/system/mongodb.service
  systemctl enable mongodb.service
  chown -R mongodb:mongodb /var/lib/mongodb
  chown -R mongodb:mongodb /var/log/mongodb
  systemctl start mongodb
fi

# C driver.
apt-get install git gcc automake autoconf libtool
if [ ! -d mongo-c-driver ]; then
  git clone https://github.com/mongodb/mongo-c-driver.git
  if [ $? -ne 0 ]; then
    echo "Could not clone mongo-c-driver!!! Please try again later..."
    exit 1
  fi
fi
cd mongo-c-driver \
 && git checkout r1.3 \
 && ./autogen.sh --prefix=/usr/local \
 && make \
 && make install \
 && cd ..
if [ $? -ne 0 ]; then
  echo "Error installing mongo-c-driver!!! Please review the errors above"
  exit 1
fi

# Upgrade CMake.
apt-get install -y software-properties-common
add-apt-repository -y ppa:george-edison55/cmake-3.x
apt-get -y update
apt-get install -y cmake
if [ $? -ne 0 ]; then
  echo "Error installing cmake-3.x!!! Please review the errors above"
  exit 1
fi
apt-get -y upgrade

# C++ driver.
if [ ! -d mongo-cxx-driver ]; then
  git clone -b master https://github.com/mongodb/mongo-cxx-driver
  if [ $? -ne 0 ]; then
    echo "Could not clone mongo-cxx-driver!!! Please try again later..."
    exit 1
  fi
fi
cd mongo-cxx-driver \
 && git checkout r3.0.0 \
 && git checkout legacy \
 && apt-get install scons \
 && scons --prefix=/usr/local --c++11=on --ssl --disable-warnings-as-errors=on install \
 && cd ..

if installCheck "$0"; then
  echo "MongoDB and C++ driver installed";
  exit 0;
else
  echo "Failed to install MongoDB and C++ driver";
  exit 1;
fi
