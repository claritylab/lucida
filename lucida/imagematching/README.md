# Image Matching (IMM)

The current implementation of IMM uses [OpenCV](http://opencv.org/), an open-source BSD-licensed library 
that includes several hundreds of computer vision algorithms. 

## Notes:

1. `opencv_imm` contains the implementation of the OpenCV IMM service. 

2. If you want to create and use another IMM implementation,
you can start by making a directory parallel to `opencv_asr` and modify `Makefile`.
Make sure to reference `../lucidaservice.thrift` and `../lucidatypes.thrift`.

3. Type `make` to build all IMM implementations,
or type `cd opencv_imm` and `make` to only build the OpenCV IMM service.
