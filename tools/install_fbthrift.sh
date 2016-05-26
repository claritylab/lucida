if [ -d "fbthrift" ]; then
  echo "FBThrift already installed, skipping"
  exit
fi

git clone https://github.com/facebook/fbthrift.git
cd fbthrift
git clone https://github.com/facebook/folly.git
cd folly/folly
autoreconf -ivf
./configure
make
make check
sudo make install
cd ../../
git clone https://github.com/facebook/wangle.git
cd wangle/wangle
cmake .
make
ctest
sudo make install
cd ../../
cd thrift/
./build/deps_ubuntu_14.04.sh  
autoreconf -if && ./configure && make
sudo make install
rm -rf .git
