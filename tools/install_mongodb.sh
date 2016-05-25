export MONGO_C_DRIVER_VERSION=1.3.0

sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv EA312927
echo "deb http://repo.mongodb.org/apt/ubuntu trusty/mongodb-org/3.2 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-3.2.list
sudo apt-get update
sudo apt-get install -y mongodb-org
sudo service mongod start

if [ -d mongo-c-driver-$MONGO_C_DRIVER_VERSION ]; then
  echo "MongoDB C Driver already installed, skipping"
  exit
fi

wget https://github.com/mongodb/mongo-c-driver/releases/download/1.3.0/mongo-c-driver-1.3.0.tar.gz
tar xzf mongo-c-driver-1.3.0.tar.gz
cd mongo-c-driver-1.3.0
./configure --with-libbson=bundled
make
sudo make install




git clone -b master https://github.com/mongodb/mongo-cxx-driver
cd mongo-cxx-driver
git checkout r3.0.0

# Upgrade CMake.
sudo apt-get install -y software-properties-common
sudo add-apt-repository -y ppa:george-edison55/cmake-3.x
sudo apt-get update
sudo apt-get install -y cmake
sudo apt-get upgrade

git checkout legacy
sudo apt-get install scons
sudo scons --prefix=/usr/local --ssl install