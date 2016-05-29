export OPENCV_VERSION=2.4.9

if [ -d /usr/local/include/opencv2 ]; then
  echo "OpenCV already installed, skipping"
  exit 0
fi

git clone https://github.com/Itseez/opencv.git opencv-$OPENCV_VERSION \
  && cd opencv-$OPENCV_VERSION \
  && git checkout $OPENCV_VERSION \
  && mkdir build \
  && cd build \
  && cmake ..  \
  && make -j$THREADS \
  && sudo make -j$THREADS install
  && rm -rf .git
