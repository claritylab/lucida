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

## Run

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
make start_test
```

Alternatively,

```
cd test
./imm_client (num_images_in_current_directory)
```
