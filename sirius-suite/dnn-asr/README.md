## DNN (ASR) kernel

Yiping Kang (ypkang@umich.edu)

University of Michgan, 2015

This is a DNN based Automatic Speech Recognition (ASR) kernel executing one
forward pass. The kernel takes voice feature vectors as input and generates
probabilities as output.  

### Backend
The kernel uses [Caffe](https://github.com/BVLC/caffe) for the DNN forward
pass.  
Build Caffe using [OpenBlas](https://github.com/xianyi/OpenBLAS) to run the
multithreaded version of this kernel.

### Directory structure
`./model/` contains the network configuration file and pre-trained model file.  
`./input/` contains an input file of features and the corresponding expected
output file.

### Input
The input included is a sentence of 548 feature vectors each of which consists
of 440 floating numbers.

### Running the kernel
1. Download and build [Caffe](https://github.com/BVLC/caffe) and
   [OpenBlas](https://github.com/xianyi/OpenBLAS).
2. Build the kernels using `make`
3. Download pre-trained model at TODO
4. Execute the kernel:  
```bash
$ make test
```
or
```bash
$ ./dnn-asr path_to_network_file path_to_pretrained_model path_to_feature_input
```
The kernel does the following:
  1. Initiates the model with weights from the pretrained model file  
  2. Loads in feature input  
  3. Executes a DNN forward pass  

You can download larger inputs at: TODO
