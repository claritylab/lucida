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
 * @author: Johann Hauswald, Yiping Kang
 * @contact: jahausw@umich.edu, ypkang@umich.edu
 */
#ifndef TONIC_H
#define TONIC_H

#include <vector>
#include "caffe/caffe.hpp"

using caffe::Blob;
using caffe::Caffe;
using caffe::Net;

using namespace std;

// largest len: FACE + \0
#define MAX_REQ_SIZE 5

struct TonicPayload {
  // Payload to DNN
  // req name
  char req_name[MAX_REQ_SIZE];
  // total size of contents
  int size;
  // number to send (1 or batched)
  int num;
  // data
  void *data;
};

struct TonicSuiteApp {
  // Tonic App information
  // task
  std::string task;
  // network config
  std::string network;
  // pretrained weights
  std::string weights;
  // file with inputs
  std::string input;
  // use GPU?
  bool gpu;

  // internal net
  Net<float> *net;

  // use DjiNN service
  bool djinn;
  // hostname to send to
  std::string hostname;
  // port
  int portno;
  // socket descriptor
  int socketfd;

  // data to send to DjiNN service
  TonicPayload pl;
};

void reshape(Net<float> *net, int input_size);

#endif
