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
#include <glog/logging.h>

#include "caffe/caffe.hpp"
#include "../../timer/timer.h"

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

void dnn_fwd(float* in, int in_size, float *out, int out_size, Net<float>* net)
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
}

int dnn_init(Net<float>* net, string weights)
{
  net->CopyTrainedLayersFrom(weights);
  return 0;
}

int load_features(float** in, string feature_file, int vec_size)
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
  if (argc < 4) {
    fprintf(stderr, "[ERROR] Input file required.\n\n");
    fprintf(stderr, "Usage: %s [NETWORK] [WEIGHTS] [INPUT FEATURES]\n\n", argv[0]);
    exit(0);
  }
  
  // turn off caffe's logging
  FLAGS_minloglevel = google::ERROR;

  STATS_INIT ("kernel", "dnn_automatic_speech_recognition");
  PRINT_STAT_STRING ("abrv", "dnn_asr");

  string network(argv[1]);
  string weights(argv[2]);
  string features(argv[3]);

  PRINT_STAT_STRING ("model", network.c_str());
  PRINT_STAT_STRING ("weights", weights.c_str());
  
  // Load DNN model
  Net<float>* dnn = new Net<float>(network);
  dnn_init(dnn, weights);
  
  // Read in feature from file
  float* feature_input;
  int feat_cnt = load_features(&feature_input, features, 440);

  PRINT_STAT_INT ("in_features", feat_cnt);

  // Perform dnn forward pass
  // FIXME: can we give 440 and 1706 a name, like MFCC_SIZE?
  int in_size = feat_cnt * 440;
  int out_size = feat_cnt * 1706; 
  float* dnn_output = (float*)malloc(sizeof(float) * out_size);

  tic ();
  dnn_fwd(feature_input, in_size, dnn_output, out_size, dnn);
  PRINT_STAT_DOUBLE ("dnn_asr", toc());

  // TODO: see issue #9
  // // Read in the correct result to sanity check
  // std::string result_file = "nnet.out";
  // float* correct_out;
  // int correct_out_cnt = load_features(&correct_out, result_file, 1706);
  //
  // // First check the numbers of vectors are same
  // assert(correct_out_cnt == feat_cnt && "The number of results are incorrect.\n");
  //
  // // Then check the number actually agrees
  // for(int i = 0; i < feat_cnt * 1706; i++){
  //   assert(isEqual(correct_out[i], dnn_output[i]) && "Results do not agree.\n");
  // }

  STATS_END ();

  return 0;
}
