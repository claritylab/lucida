#!/bin/bash
if [ -d localpython2_7_12 ];
then
  echo "Virtual Python environment installed."
  exit 0
fi
wget http://www.python.org/ftp/python/2.7.12/Python-2.7.12.tgz
mkdir localpython2_7_12
tar -zxvf Python-2.7.12.tgz
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
pip install --upgrade -r python_requirements.txt
deactivate
