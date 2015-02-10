## [Sirius-suite IMM]: Feature Description

This is the Feature Description (fd) kernel, the second step of the image
matching pipeline in Sirius, receives image keypoints which are clustered into
robust descriptors representing interesting areas of the image.

### Backend
The feature description uses OpenCV''s
[SURF](http://docs.opencv.org/modules/nonfree/doc/feature_detection.html#surf)
baseline and GPU implementation. The Pthreaded version tiles the image and each
thread is responsible for a piece (or multiple pieces) of the iamge.

### Directory structure
`./input/` contains images of various sizes. When using more threads, consider using a
larger image.

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
$ ./surf-fd ../input/2048x2048.jpg
```
The kernel does the following:
    1. (pthread) The image is tiled in function of how many threads are allocated,
    2. SURF FE is called on the image to generate keypoints,
    3. SURF FD is called on the keypoints to generate feature descriptors.
