#!/bin/bash
installCheck () {
  g++ check_opencv.cpp -o check_opencv
  if [[ $? -ne 0 ]]; then
    return 1
  else
    rm check_opencv
    return 0
  fi
}

if installCheck "$0"; then
  echo "OpenCV already installed, skipping"
  exit 0
fi

git clone https://github.com/opencv/opencv.git opencv-2.4 \
  && cd "opencv-2.4" \
  && git checkout "2.4" \
  && mkdir build \
  && cd build \
  && cmake ..  \
  && make -j"$THREADS" \
  && sudo make -j"$THREADS" install \
  && cd ../ \
  && rm -rf .git \
  && cd ../

if installCheck "$0"; then
  echo "OpenCV installed";
  exit 0;
else
  echo "Failed to install OpenCV";
  exit 1;
fi
