if [ -d localpython2_7_9 ];
then
  echo "Virtual Python environment installed."
  exit 0
fi
wget http://www.python.org/ftp/python/2.7.9/Python-2.7.9.tgz
mkdir localpython2_7_9
tar -zxvf Python-2.7.9.tgz
cd Python-2.7.9
./configure --prefix=$(pwd)/../localpython2_7_9
make
make install
cd ../
virtualenv python_2_7_9 -p localpython2_7_9/bin/python2.7
source python_2_7_9/bin/activate
pip install --upgrade distribute
pip install --upgrade pip
pip install -r python_requirements.txt
deactivate
