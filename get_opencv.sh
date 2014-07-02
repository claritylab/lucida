# jahausw@umich.edu
# 2014
# Get correct opencv
sudo apt-get install build-essential checkinstall git cmake libfaac-dev libjack-jackd2-dev libmp3lame-dev libopencore-amrnb-dev libopencore-amrwb-dev libsdl1.2-dev libtheora-dev libva-dev libvdpau-dev libvorbis-dev libx11-dev libxfixes-dev libxvidcore-dev texi2html yasm zlib1g-dev

opendir=opencv-2.4.9
wget http://downloads.sourceforge.net/project/opencvlibrary/opencv-unix/2.4.9/opencv-2.4.9.zip
unzip $opendir; cd $opendir; mkdir build;

cmake -DCMAKE_INSTALL_PREFIX:PATH=`pwd`/build
make -j 4
make install

cd ..;  rm -rf opencv-2.4.9.zip
