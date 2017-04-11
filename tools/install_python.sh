#!/bin/bash
if [ ! -d Python-2.7.12 ]; then
  wget -c http://www.python.org/ftp/python/2.7.12/Python-2.7.12.tgz && tar -zxvf Python-2.7.12.tgz
  if [ $? -ne 0 ]; then
    echo "Could not download Python!!! Please try again later..."
    exit 1
  fi
fi
mkdir -p localpython2_7_12
cd Python-2.7.12
./configure --prefix="$(pwd)"/../localpython2_7_12
make
make install
cd ../
virtualenv python_2_7_12 -p localpython2_7_12/bin/python2.7
source python_2_7_12/bin/activate
pip install --upgrade distribute
pip install --upgrade pip
pip install -r python_requirements.txt
deactivate
