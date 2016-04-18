export OPENCV_VERSION=2.4.9

if [ -z "$THREADS" ]; then
  THREADS=4
fi

if [ -d opencv-$OPENCV_VERSION ]; then
  echo "OpenCV already installed, skipping"
  exit
fi

git clone https://github.com/Itseez/opencv.git opencv-$OPENCV_VERSION \
  && cd opencv-$OPENCV_VERSION \
  && git checkout $OPENCV_VERSION \
  && mkdir build \
  && cd build \
  && cmake ..  \
  && make -j$THREADS \
  && sudo make -j$THREADS install
