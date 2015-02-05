## DNN(ASR) kernel

Yiping Kang (ypkang@umich.ed)

University of Michgan, 2015

What's this
==========
This kernel is a forward pass of a deep neural network (DNN).
The network is for Automatic Speech Recognition (ASR).
The kernel takes voice feature vectors as input and generate probability as output.

Directory structure
===================
./model/ contains the network configuration file and pre-trained model file.
./input/ contains input file of features and and corresponding expected output file for sanity check purpose.

Backend
======
The kernel use Caffe (Cite. License ?) as its neural network framework.

How to run the kernel
====================
Follow the following steps to run this kernel
1. Download Caffe at https://github.com/BVLC/caffe
2. Compile Caffe by typing `make distribute` in $(Caffe)/src
3. Change the first line in Makefile to $(Caffe)/distribute
4. You may also need to change the CUDA pointer in Makefile
5. Type 'make' in the version of the kernel you want (e.g baseline, GPU, etc)
6. Execute the kernel by
  $ ./dnn-asr path_to_model_file path_to_feature_input
For example, if you keep the folder as default,
  $ ./dnn-asr model/asr.caffemodel input/feature.in
7. The kernel will do the following
  a. Initiate the model with weights in the pretrained model file
  b. Load in feature input
  c. DNN performs forward pass
  d. Check output with the result file in input/ 

Input
=====
The input included is a sentence with normal length and it contains 548 feature vectors and each of which consists 440 floating numbers.

You can download larger inputs at:
TODO
