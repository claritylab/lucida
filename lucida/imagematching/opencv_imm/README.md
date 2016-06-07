# OpenCV IMM

OpenCV IMM uses [OpenCV](http://opencv.org/), an open-source BSD-licensed library 
that includes several hundreds of computer vision algorithms. 

## Structure

- `server/`: implementation of the IMM server
- `test/`: implementation of the IMM testing client

## Build

```
$ make
```

## Run and Test

Start the server:

```
make start_server
```

Alternatively,

```
cd server
./imm_server
```

## Test


```
$ cd test
$ ./imm_client (port number of IMM) (path of image file **relative to test**)
``` 

Alternatively,
```
$ make start_test (port number of IMM) (path of image file **relative to test**)
```

An example image file is provided `test.jpg`.

## Summary: Example Usage

```
$ make
$ make start_server 8081
$ # Wait until you see "Start listening to requests" in the server terminal.
$ make start_test 8081 test.jpg
```
