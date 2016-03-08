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
#include <fstream>
#include <sstream>
#include <iostream>
#include <assert.h>
#include <stdint.h>
#include <ctime>
#include <cmath>
#include <glog/logging.h>
#include <boost/chrono/thread_clock.hpp>

#include "timer.h"
#include "thread.h"

extern map<string, Net<float>*> nets;
extern bool debug;
extern bool gpu;

void SERVICE_fwd(float* in, int in_size, float* out, int out_size,
                 Net<float>* net) {
  string net_name = net->name();
  STATS_INIT("service", "DjiNN service inference");
  PRINT_STAT_STRING("network", net_name.c_str());

  if (Caffe::mode() == Caffe::CPU)
    PRINT_STAT_STRING("platform", "cpu");
  else
    PRINT_STAT_STRING("platform", "gpu");

  float loss;
  vector<Blob<float>*> in_blobs = net->input_blobs();

  tic();
  in_blobs[0]->set_cpu_data(in);
  vector<Blob<float>*> out_blobs = net->ForwardPrefilled(&loss);
  memcpy(out, out_blobs[0]->cpu_data(), sizeof(float));

  PRINT_STAT_DOUBLE("inference latency", toc());

  STATS_END();

  if (out_size != out_blobs[0]->count())
    LOG(FATAL) << "out_size =! out_blobs[0]->count())";
  else
    memcpy(out, out_blobs[0]->cpu_data(), out_size * sizeof(float));
}

pthread_t request_thread_init(int sock) {
  // Prepare to create a new pthread
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 1024 * 1024);

  // Create a new thread starting with the function request_handler
  pthread_t tid;
  if (pthread_create(&tid, &attr, request_handler, (void*)(intptr_t)sock) != 0)
    LOG(ERROR) << "Failed to create a request handler thread.\n";

  return tid;
}

void* request_handler(void* sock) {
  int socknum = (intptr_t)sock;

  // 1. Client sends the application type
  // 2. Client sends the size of incoming data
  // 3. Client sends data

  char req_name[MAX_REQ_SIZE];
  SOCKET_receive(socknum, (char*)&req_name, MAX_REQ_SIZE, debug);
  map<string, Net<float>*>::iterator it = nets.find(req_name);
  if (it == nets.end()) {
    LOG(ERROR) << "Task " << req_name << " not found.";
    return (void*)1;
  } else
    LOG(INFO) << "Task " << req_name << " forward pass.";

  // receive the input data length (in float)
  int sock_elts = SOCKET_rxsize(socknum);
  if (sock_elts < 0) {
    LOG(ERROR) << "Error num incoming elts.";
    return (void*)1;
  }

  // reshape input dims if incoming data != current net config
  LOG(INFO) << "Elements received on socket " << sock_elts << endl;

  reshape(nets[req_name], sock_elts);

  int in_elts = nets[req_name]->input_blobs()[0]->count();
  int out_elts = nets[req_name]->output_blobs()[0]->count();
  float* in = (float*)malloc(in_elts * sizeof(float));
  float* out = (float*)malloc(out_elts * sizeof(float));

  // Main loop of the thread, following this order
  // 1. Receive input feature (has to be in the size of sock_elts)
  // 2. Do forward pass
  // 3. Send back the result
  // 4. Repeat 1-3

  // Warmup: used to move the network to the device for the first time
  // In all subsequent forward passes, the trained model resides on the
  // device (GPU)
  bool warmup = true;

  while (1) {
    LOG(INFO) << "Reading from socket.";
    int rcvd =
        SOCKET_receive(socknum, (char*)in, in_elts * sizeof(float), debug);

    if (rcvd == 0) break;  // Client closed the socket

    if (warmup && gpu) {
      float loss;
      vector<Blob<float>*> in_blobs = nets[req_name]->input_blobs();
      in_blobs[0]->set_cpu_data(in);
      vector<Blob<float>*> out_blobs;
      out_blobs = nets[req_name]->ForwardPrefilled(&loss);
      warmup = false;
    }

    LOG(INFO) << "Executing forward pass.";
    SERVICE_fwd(in, in_elts, out, out_elts, nets[req_name]);

    LOG(INFO) << "Writing to socket.";
    SOCKET_send(socknum, (char*)out, out_elts * sizeof(float), debug);
  }

  // Exit the thread
  LOG(INFO) << "Socket closed by the client.";

  free(in);
  free(out);

  return (void*)0;
}
