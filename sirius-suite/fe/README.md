## [Sirius-suite IMM]: Feature Extraction

This is the Feature Extraction (fe) kernel, the first step of the image
matching pipeline in Sirius, extracts interesting keypoints from the input
image.

### Backend
The feature extraction uses OpenCV''s
[SURF](http://docs.opencv.org/modules/nonfree/doc/feature_detection.html#surf)
baseline and GPU implementation. The Pthreaded version tiles the image and each
thread is responsible for a piece (or multiple pieces) of the iamge.

### Directory structure
`./input/` contains images of various sizes. When using more threads, consider
using a larger image.

### Running the kernel
1. Build all:  
```bash
$ make
```
2. Run the kernel on all platforms with the default inputs:
```bash
$ make test
```
or
```bash
$ ./surf-fe ../input/2048x2048.jpg
```
The kernel does the following:
    1. (pthread) The image is tiled in function of how many threads are allocated,
    2. SURF FE is called on the image to generate keypoints.
