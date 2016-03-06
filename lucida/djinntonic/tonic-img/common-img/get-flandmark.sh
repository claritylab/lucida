# library needed for (optional) facial alignment preprocessing step
# download Flandmark library and install

if [ ! -f flandmark.zip ]
then
  wget http://web.eecs.umich.edu/~jahausw/download/flandmark.zip
  unzip flandmark.zip
  cd flandmark-master
  cmake .
  make
  cp libflandmark/libflandmark_static.a ../
fi
