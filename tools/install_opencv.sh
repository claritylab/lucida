export OPENCV_VERSION=2.4.9

if [ -z "$THREADS" ]; then
  THREADS=4
fi

git clone https://github.com/Itseez/opencv.git opencv-$OPENCV_VERSION \
  && cd opencv-$OPENCV_VERSION \
  && git checkout $OPENCV_VERSION \
  && mkdir build \
  && cd build \
  && cmake .. -DCMAKE_INSTALL_PREFIX=../install \
  && make -j$THREADS \
  && make -j$THREADS install
