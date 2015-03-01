MAKE="make --jobs=$NUM_THREADS"

wget http://github.com/xianyi/OpenBLAS/tarball/v0.2.13
tar xzf v0.2.13
cd xianyi-OpenBLAS-aceee4e
$MAKE
$MAKE install
