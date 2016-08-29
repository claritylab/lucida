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

if installCheck $0; then
  echo "MongoDB and C++ driver installed";
  exit 0;
fi

# MongoDB.
sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv EA312927
echo "deb http://repo.mongodb.org/apt/ubuntu trusty/mongodb-org/3.2 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-3.2.list
sudo apt-get update
sudo apt-get install -y mongodb-org
sudo service mongod start

# C driver.
sudo apt-get install git gcc automake autoconf libtool
git clone https://github.com/mongodb/mongo-c-driver.git
cd mongo-c-driver
git checkout r1.3
./autogen.sh --prefix=/usr/local
make
sudo make install
rm -rf .git
cd ..

git clone git://github.com/mongodb/libbson.git
cd libbson/
git checkout r1.3
./autogen.sh
make
sudo make install
rm -rf .git
cd ..

# Upgrade CMake.
sudo apt-get install -y software-properties-common
sudo add-apt-repository -y ppa:george-edison55/cmake-3.x
sudo apt-get -y update
sudo apt-get install -y cmake
sudo apt-get -y upgrade

# C++ driver.
git clone -b master https://github.com/mongodb/mongo-cxx-driver
cd mongo-cxx-driver
git checkout r3.0.0
git checkout legacy
sudo apt-get install scons
sudo scons --prefix=/usr/local --c++11=on --ssl install
rm -rf .git
cd ..

if installCheck $0; then
  echo "MongoDB and C++ driver installed"; 
  exit 0;
else 
  echo "Faile to install MongoDB and C++ driver"; 
fi
