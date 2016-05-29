export MONGO_C_DRIVER_VERSION=1.3.0

installCheck () {
  if [[ $(g++ mongodb_check.cpp -I /usr/local/include/mongo /usr/local/lib/libmongoclient.a  -lboost_thread -lboost_filesystem -lboost_program_options -lboost_system -pthread -o mongodb_check) ]]; then
  	return 1
  fi
  if [[ $(./mongodb_check) == "Connection ok" ]]; then
    rm mongodb_check
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

(mongo --version) | grep "MongoDB shell version:" &> /dev/null
if [ $? != 0 ]; then
  echo 'Failed to install MongoDB'
  exit 1
fi

# C driver.
wget https://github.com/mongodb/mongo-c-driver/releases/download/1.3.0/mongo-c-driver-1.3.0.tar.gz
tar xzf mongo-c-driver-1.3.0.tar.gz
cd mongo-c-driver-1.3.0
./configure --with-libbson=bundled
make
sudo make install
rm -rf .git

# Upgrade CMake.
sudo apt-get install -y software-properties-common
sudo add-apt-repository -y ppa:george-edison55/cmake-3.x
sudo apt-get update
sudo apt-get install -y cmake
sudo apt-get upgrade

# C++ driver.
git clone -b master https://github.com/mongodb/mongo-cxx-driver
cd mongo-cxx-driver
git checkout r3.0.0
git checkout legacy
sudo apt-get install scons
sudo scons --prefix=/usr/local --ssl install
rm -rf .git

if installCheck $0; then
  echo "MongoDB and C++ driver installed"; 
  exit 0;
else 
  echo "Faile to install Facebook Thrift"; 
  exit 1;
fi
