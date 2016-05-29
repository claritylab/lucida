installCheck () {
  python -mthrift_compiler.main --gen cpp2 ../lucida/lucidaservice.thrift
  if [ -d gen-cpp2 ]; then
    rm -rf gen-cpp2
    return 0
  else
    return 1
  fi
}

if installCheck $0; then
  echo "Facebook Thrift installed";
  exit 0;
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
cd ../
rm -rf .git
cd ../
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
cd ..
rm -rf .git

if installCheck $0; then
  echo "Facebook Thrift installed";
  exit 0;
else
  echo "Faile to install Facebook Thrift";
  exit 1;
fi
