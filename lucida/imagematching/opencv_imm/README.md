# OpenCV IMM

OpenCV IMM uses [OpenCV](http://opencv.org/), an open-source BSD-licensed library 
that includes several hundreds of computer vision algorithms. 

## Major Dependencies

- [OpenCV](http://opencv.org/)
- [Facebook Thrift](https://github.com/facebook/fbthrift)
- [MongoDB](https://www.mongodb.com/)
 and [C++ legacy driver](https://github.com/mongodb/mongo-cxx-driver/tree/legacy)

# Structure

- `server/`: implementation of the IMM server
- `test/`: implementation of the IMM testing client

## Build

```
make
```

## Run

Start the server:

```
make start_server
```

Wait until you see `IMM at 8082`.

Alternatively,

```
cd server
./imm_server
```

## Test

```
make start_test
```

Alternatively,

```
cd test
./imm_client (num_images)
```

7 images `test/test*.jpg` are provided.

## Developing Notes

1. The linker flags in `server/Makefile` are complicated and should be modified with caution.
Specifically, `-lmongoclient` should precede `-lssl` and `-lcrypto`.

2. `server/Image.cpp` uses `cv::SurfFeatureDetector` to turn
an image into a descriptor matrix both represented as `std::string`,
and `server/IMMHandler.cpp` saves the matrix into and loads it from
[GridFS](https://docs.mongodb.com/manual/core/gridfs/).
