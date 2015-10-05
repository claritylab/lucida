# run to extract tars needed:
# extracts:
#   GMM input data
#   ASR pretrained model

#run as sudo

# Get Caffe Suite and set up
wget http://web.eecs.umich.edu/~jahausw/download/sirius-caffe.tar.gz

tar xzf sirius-caffe.tar.gz
cd sirius-caffe;
sudo ./get-opencv.sh
sudo ./get-libs.sh
#Get the missing (from gitorious: RIP) mdb that is cloned on github 
git clone https://github.com/wizawu/lmdb.git
cd lmdb/libraries/liblmdb
make && make install


sudo apt-get install -y python-numpy

sudo ./make-and-install.sh

cd ..

#Original Preparations

find . -name '*.tar.gz' -exec sh -c 'tar -xzvf {} -C $(dirname {})' \;
