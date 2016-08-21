export OPENCV_VERSION=2.4.9

installCheck () {
  if [[ $(g++ check_opencv.cpp -o check_opencv) ]]; then
    return 1
  else
    rm check_opencv
    return 0
  fi
}

if installCheck $0; then
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
  && sudo make -j$THREADS install \
  && rm -rf .git

if installCheck $0; then
  echo "OpenCV installed"; 
  exit 0;
else 
  echo "Faile to install OpenCV";
  exit 1;
fi
