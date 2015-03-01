MAKE="make --jobs=$NUM_THREADS"

apt-get install gfortran

wget http://github.com/xianyi/OpenBLAS/tarball/v0.2.13
tar xzf v0.2.13
cd xianyi-OpenBLAS-aceee4e
$MAKE 1 > /dev/null
$MAKE PREFIX=/usr/local/lib install
