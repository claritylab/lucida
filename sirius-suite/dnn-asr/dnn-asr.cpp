/*
 *  Copyright (c) 2015, University of Michigan.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

/**
 * DNN for speech recognition decoding
 *
 * @author: Yiping Kang
 * @contact: ypkang@umich.edu
 */

#include <assert.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <cmath>

#include "caffe/caffe.hpp"

using caffe::Blob;
using caffe::Caffe;
using caffe::Net;
using caffe::Layer;

using namespace std;

inline bool isEqual(float x, float y)
{
  const float epsilon = pow(10, -4);
  return std::abs(x-y)<=epsilon;
}

int dnn_fwd(float* in, int in_size, float *out, int out_size, Net<float>* net)
{
  vector<Blob<float>* > in_blobs = net->input_blobs();

  vector<Blob<float>* > out_blobs;

  float loss;

  // Perform DNN forward pass
  in_blobs[0]->set_cpu_data(in);
  out_blobs = net->ForwardPrefilled(&loss);

  assert(out_size == out_blobs[0]->count() 
      && "Output size not expected.");

  memcpy(out, out_blobs[0]->cpu_data(), sizeof(float) * out_size);

  return 0;
}

int dnn_init(Net<float>* net, string model_file)
{
  net->CopyTrainedLayersFrom(model_file);
  return 0;
}

int load_feature(float** in, string feature_file, int vec_size)
{
  // Read in features from file
  // First need to detect how many feature vectors 
  std::ifstream inFile(feature_file.c_str(), std::ifstream::in);
  int feat_cnt = std::count(std::istreambuf_iterator<char>(inFile),
      std::istreambuf_iterator<char>(), '\n') - 1;
  
  // Allocate memory for input feature array
  *in = (float*)malloc(sizeof(float) * feat_cnt * vec_size); 

  // Read the feature in
  int idx = 0;
  std::ifstream featFile(feature_file.c_str(), std::ifstream::in);
  std::string line;
  std::getline(featFile, line); // Get rid of the first line
  while(std::getline(featFile, line)){
    std::istringstream iss(line);
    float temp;
    while(iss >> temp){
      (*in)[idx] = temp;
      idx++;
    }
  }
  // Everything should be in, check for sure
  assert(idx == feat_cnt * vec_size && "Error: Read-in feature not correct.");

  return feat_cnt;
}

int main(int argc, char** argv)
{
  // Main function of DNN forward pass for speech recognition
  // Prase arguments
  if(argc != 3){
    printf("Usage: ./dnn-asr path_to_model path_to_input_feature\n");
    exit(1);
  }

  std::string model_file(argv[1]);
  std::string feature_file(argv[2]);
  
  printf("Decoding on input in %s with model in %s.\n", feature_file.c_str(), model_file.c_str());

  // Load DNN model
  Net<float>* dnn = new Net<float>("asr.prototxt");
  dnn_init(dnn, model_file);
  
  // Read in feature from file
  float* feature_input;
  int feat_cnt = load_feature(&feature_input, feature_file, 440);
  printf("%d feature vectors found in the input.\n", feat_cnt);

  // Perform dnn forward pass
  int in_size = feat_cnt * 440;
  int out_size = feat_cnt * 1706; 
  float* dnn_output = (float*)malloc(sizeof(float) * out_size);
  if(dnn_fwd(feature_input, in_size, dnn_output, out_size, dnn) != 0){
    printf("Error while DNN decoding. Abort.\n");
    exit(1);
  }

  printf("One forward pass is done.\n");

  // TODO: Result check
  // Read in the correct result to sanity check
  std::string result_file = "nnet.out";
  float* correct_out;
  int correct_out_cnt = load_feature(&correct_out, result_file, 1706);

  // First check the numbers of vectors are same
  assert(correct_out_cnt == feat_cnt && "The number of results are incorrect.\n");

  // Then check the number actually agrees
  for(int i = 0; i < feat_cnt * 1706; i++){
    assert(isEqual(correct_out[i], dnn_output[i]) && "Results do not agree.\n");
  }

  printf("Sanity check passed !\n");

  return 0;
}
