# OpenCV IMM

OpenCV IMM uses [OpenCV](http://opencv.org/), an open-source BSD-licensed library 
that includes several hundreds of computer vision algorithms. 

This directory contains three sub-directories:

```
opencv/ 
server/
test/
```

- `opencv/`: Contains the code that interacts with OpenCV
- `server/`: Contains the main program that listens to requests
- `test/`: Contains a testing client

## Build

```
$ make
```

## Run and Test

Start the server:

```
$ cd server
$ ./imm_server (port number of IMM) (port number of command center, optional)
```

Alternatively,
```
$ make start_server (port number of IMM) (port number of command center, optional)
```

Note: There are two modes of usage. 
* If the port number of the command center is not provided,
or the command center cannot be connected to,
the server runs as a stand-alone program.
* Otherwise, the server can interact with the command center
and act as its client.

In either case, the server can interact with a testing client.
To run the testing client:

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
