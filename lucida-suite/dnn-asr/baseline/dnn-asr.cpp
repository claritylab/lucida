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
#include <cblas.h>

#include "caffe/caffe.hpp"

#include "../../utils/memoryman.h"
#include "../../utils/timer.h"
#include "../../utils/memoryman.h"

#define FEATURE_VEC_SIZE 440  // Number of floats in one input feature vector
#define PROB_VEC_SIZE 1706  // Number of floats in one output probability vector

using caffe::Blob;
using caffe::Caffe;
using caffe::Net;
using caffe::Layer;

using namespace std;

inline bool isEqual(float x, float y) {
  const float epsilon = pow(10, -4);
  return abs(x - y) <= epsilon;
}

void dnn_fwd(float* in, int in_size, float* out, int out_size,
             Net<float>* net) {
  vector<Blob<float>*> in_blobs = net->input_blobs();
  vector<Blob<float>*> out_blobs;

  float loss;

  // Perform DNN forward pass
  in_blobs[0]->set_cpu_data(in);
  out_blobs = net->ForwardPrefilled(&loss);

  assert(out_size == out_blobs[0]->count() && "Output size not expected.");

  memcpy(out, out_blobs[0]->cpu_data(), sizeof(float) * out_size);
}

int dnn_init(Net<float>* net, string weights) {
  net->CopyTrainedLayersFrom(weights);
  return 0;
}

int load_features(float** in, string feature_file, int vec_size) {
  // Read in features from file
  // First need to detect how many feature vectors
  ifstream inFile(feature_file.c_str(), ifstream::in);
  int feat_cnt = count(istreambuf_iterator<char>(inFile),
                       istreambuf_iterator<char>(), '\n') -
                 1;

  // Allocate memory for input feature array
  *in = (float*)sirius_malloc(sizeof(float) * feat_cnt * vec_size);

  // Read the feature in
  int idx = 0;
  ifstream featFile(feature_file.c_str(), ifstream::in);
  string line;
  getline(featFile, line);  // Get rid of the first line
  while (getline(featFile, line)) {
    istringstream iss(line);
    float temp;
    while (iss >> temp) {
      (*in)[idx] = temp;
      idx++;
    }
  }

  // Everything should be in, check for sure
  assert(idx == feat_cnt * vec_size && "Error: Read-in feature not correct.");

  return feat_cnt;
}

int main(int argc, char** argv) {
  if (argc < 4) {
    fprintf(stderr, "[ERROR] Input file required.\n\n");
    fprintf(stderr, "Usage: %s [NETWORK] [WEIGHTS] [INPUT FEATURES]\n\n",
            argv[0]);
    exit(0);
  }

  // turn off caffe's loggingk
  FLAGS_minloglevel = google::ERROR;

  STATS_INIT("kernel", "dnn_automatic_speech_recognition");
  PRINT_STAT_STRING("abrv", "dnn-asr");

  string network(argv[1]);
  string weights(argv[2]);
  string features(argv[3]);

  PRINT_STAT_STRING("model", network.c_str());
  PRINT_STAT_STRING("weights", weights.c_str());

  // Load DNN model
  Net<float>* dnn = new Net<float>(network);
  dnn_init(dnn, weights);

  // Read in feature from file
  float* feature_input;
  int feat_cnt = load_features(&feature_input, features, FEATURE_VEC_SIZE);

  PRINT_STAT_INT("in_features", feat_cnt);

  // Perform dnn forward pass
  int in_size = feat_cnt * FEATURE_VEC_SIZE;
  int out_size = feat_cnt * PROB_VEC_SIZE;
  float* dnn_output = (float*)sirius_malloc(sizeof(float) * out_size);

  tic();
  dnn_fwd(feature_input, in_size, dnn_output, out_size, dnn);
  PRINT_STAT_DOUBLE("dnn-asr", toc());

  STATS_END();

#if TESTING
  std::string result_file = "../input/correct.out";
  float* correct_out;
  int correct_out_cnt = load_features(&correct_out, result_file, PROB_VEC_SIZE);

  // First check that the numbers of vectors are same
  assert(correct_out_cnt == feat_cnt);

  // Then check that the number actually agrees
  for (int i = 0; i < feat_cnt * PROB_VEC_SIZE; i++)
    assert(isEqual(correct_out[i], dnn_output[i]));

#endif

  return 0;
}
