installCheck () {
  if [ ! -d fbthrift ]; then
    return 1
  fi
  python -mthrift_compiler.main --gen cpp2 check_thrift.thrift
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
cd fbthrift/thrift/
git checkout d811b530a4f5e11a520f5fb416a5a3a8a5f42ef8
./build/deps_ubuntu_14.04.sh
cd ../
git clone https://github.com/facebook/folly.git
cd folly/folly
git checkout 6645b95d6402b48e7b3abdc01146a7a61a9378c9
autoreconf -ivf
./configure
make
make check
sudo make install
cd ../
# rm -rf .git
cd ../
git clone https://github.com/facebook/wangle.git
cd wangle/wangle
git checkout 86c4794422e473f3ed5b50035104e1bc04c9646d
cmake . -DBUILD_TESTS=OFF
make
ctest
sudo make install
cd ../
# rm -rf .git
cd ../
cd thrift/
autoreconf -if
./configure
sudo make
sudo make install
cd ..
# rm -rf .git
cd ..

if installCheck $0; then
  echo "Facebook Thrift installed";
  exit 0;
else
  echo "Failed to install Facebook Thrift";
  exit 1;
fi
