// nnetbin/nnet-forward.cc

// Copyright 2011-2013  Brno University of Technology (Author: Karel Vesely)

// See ../../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

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
 * @author: Yiping Kang, Johann Hauswald 
 * @contact: ypkang@umich.edu, jahausw@umich.edu
 */

#include <limits>
#include <ctime>
#include <fstream>
#include <vector>

#include "nnet/nnet-nnet.h"
#include "nnet/nnet-pdf-prior.h"
#include "nnet/nnet-rbm.h"
#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "base/timer.h"
#include "socket.h"
#include "tonic.h"

#define DEBUG 0

int main(int argc, char *argv[]) {
  using namespace kaldi;
  using namespace kaldi::nnet1;
  typedef kaldi::int32 int32;
  try {
    const char *usage =
        "Perform forward pass through Neural Network.\n"
        "\n"
        "Usage:  nnet-forward [options] <feature-rspecifier> "
        "<feature-wspecifier>\n"
        " [options] --hostname --portno"
        "e.g.: \n"
        " nnet-forward ark:features.ark ark:mlpoutput.ark\n";

    ParseOptions po(usage);

    PdfPriorOptions prior_opts;
    prior_opts.Register(&po);

    // Local inference information
    string common = "../../../../common/";
    po.Register("common", &common, "Directory with configs and weights");
    string network = "asr.prototxt";
    po.Register("network", &network, "Network config file (.prototxt)");
    string weight = "asr.caffemodel";
    po.Register("weight", &weight, "Pretrained weight (.caffemodel)");
    int batch = 1;
    po.Register("batch", &batch, "Batch size");

    // DjiNN service information
    bool djinn = false;
    po.Register("djinn", &djinn, "Use DjiNN service?");
    string hostname = "localhost";
    po.Register("hostname", &hostname, "Server IP addr");
    int portno = 8080;
    po.Register("portno", &portno, "Server port");

    // Common configuraition
    bool gpu = false;
    po.Register("gpu", &gpu, "Use GPU?");
    bool debug = false;
    po.Register("debug", &debug, "Turn on all debug");

    // ASR specific inputs and flags
    // inherited from Kaldi
    std::string feature_transform;
    po.Register("feature-transform", &feature_transform,
                "Feature transform in front of main network (in nnet format)");

    // Read in the argument
    po.Read(argc, argv);

    if (po.NumArgs() != 2) {
      po.PrintUsage();
      exit(1);
    }

    // Input to ASR
    // inherited from Kaldi
    std::string feature_rspecifier = po.GetArg(1),
                feature_wspecifier = po.GetArg(2);

    network = common + "configs/" + network;
    weight = common + "weights/" + weight;

    // Initialize tonic app
    TonicSuiteApp app;
    app.task = "asr";
    app.network = network;
    app.weights = weight;

    app.djinn = djinn;
    app.gpu = gpu;

    if (app.djinn) {
      app.hostname = hostname;
      app.portno = portno;
      app.socketfd =
          CLIENT_init((char *)app.hostname.c_str(), app.portno, debug);

      if (app.socketfd < 0) {
        exit(1);
      }
    } else {
      app.net = new Net<float>(app.network);
      app.net->CopyTrainedLayersFrom(app.weights);
      Caffe::set_phase(Caffe::TEST);
      if (app.gpu)
        Caffe::set_mode(Caffe::GPU);
      else
        Caffe::set_mode(Caffe::CPU);
    }

    // Set request type
    app.pl.size = 0;
    strcpy(app.pl.req_name, app.task.c_str());

    //    //Select the GPU
    //#if HAVE_CUDA==1
    //    CuDevice::Instantiate().SelectGpuId(use_gpu);
    //    CuDevice::Instantiate().DisableCaching();
    //#endif

    Nnet nnet_transf;
    if (feature_transform != "") {
      nnet_transf.Read(feature_transform);
    }

    PdfPrior pdf_prior(prior_opts);

    // disable dropout
    nnet_transf.SetDropoutRetention(1.0);

    SequentialBaseFloatMatrixReader feature_reader(feature_rspecifier);
    BaseFloatMatrixWriter feature_writer(feature_wspecifier);

    CuMatrix<BaseFloat> feats, feats_transf, nnet_out;
    Matrix<BaseFloat> nnet_out_host;

    Timer time;
    int32 num_done = 0;

    // iterate over all feature files
    // cumulate them for batch processing
    std::vector<float> batched_feats;
    std::vector<int> feats_row_cnt;
    std::vector<string> feature_keys;
    int offset = 0;
    int total_rows = 0;
    for (; !feature_reader.Done(); feature_reader.Next()) {
      const Matrix<BaseFloat> &mat = feature_reader.Value();

      KALDI_LOG << "Processing utterance " << num_done + 1 << ", "
                << feature_reader.Key() << ", " << mat.NumRows() << "frm";

      feature_keys.push_back(feature_reader.Key());
      // check for NaN/inf
      BaseFloat sum = mat.Sum();
      if (!KALDI_ISFINITE(sum)) {
        KALDI_ERR << "NaN or inf found in features of " << feature_reader.Key();
      }

      // Feature transformation will use GPU if possible
      feats = mat;

      // Preprocessing feature transformation
      nnet_transf.Feedforward(feats, &feats_transf);

      int cur_num_feats = feats_transf.NumRows() * feats_transf.NumCols();
      batched_feats.resize(batched_feats.size() + cur_num_feats);

      feats_row_cnt.push_back(feats_transf.NumRows());

      // Concatenate this to the total input
      for (MatrixIndexT i = 0; i < feats_transf.NumRows(); i++) {
        std::copy(feats_transf.Row(i).Data(),
                  feats_transf.Row(i).Data() + feats_transf.NumCols(),
                  batched_feats.begin() + offset);
        offset += feats_transf.NumCols();
        total_rows++;
      }
    }

    app.pl.num = total_rows;
    app.pl.size = offset / total_rows;
    app.pl.data = (float *)malloc(offset * sizeof(float));
    memcpy((char *)app.pl.data, (char *)&batched_feats[0],
           offset * sizeof(float));

    std::vector<CuMatrix<BaseFloat> > output_list;
    // Inference
    if (app.djinn) {
      KALDI_LOG << "Use DjiNN service to inference.";

      // Send request type
      SOCKET_send(app.socketfd, (char *)&app.pl.req_name, MAX_REQ_SIZE, debug);
      KALDI_LOG << "DEBUG IS " << debug;

      // Send data length
      SOCKET_txsize(app.socketfd, offset);

      // Send features
      SOCKET_send(app.socketfd, (char *)app.pl.data, offset * sizeof(float),
                  debug);

      // Receive results
      // Receive into multiple kaldi's matrix format
      int total_rcvd = 0;
      for (int feat_idx = 0; feat_idx < feats_row_cnt.size(); feat_idx++) {
        // Resize the receiving matrix
        nnet_out.Resize(feats_row_cnt[feat_idx], 1706);
        for (MatrixIndexT i = 0; i < nnet_out.NumRows(); i++) {
          int rcvd =
              SOCKET_receive(app.socketfd, (char *)nnet_out.Row(i).Data(),
                             1706 * sizeof(float), debug);
          total_rcvd += rcvd;
        }
        output_list.push_back(nnet_out);
      }
      if (total_rcvd != total_rows * 1706 * sizeof(float)) {
        KALDI_ERR << "Not receiving enought output";
        exit(1);
      }

      // Close the socket
      //      SOCKET_close(app.socketfd, debug);
      //      KALDI_LOG << "DjiNN service returns. Socket closed.";
    } else {
      // Use local inference
      KALDI_LOG << "Use local dnn service to inference.";

      float loss;
      reshape(app.net, offset);

      float *preds = (float *)malloc(total_rows * 1706 * sizeof(float));
      vector<Blob<float> *> in_blobs = app.net->input_blobs();
      in_blobs[0]->set_cpu_data((float *)app.pl.data);

      vector<Blob<float> *> out_blobs = app.net->ForwardPrefilled(&loss);

      memcpy(preds, out_blobs[0]->cpu_data(),
             total_rows * 1706 * sizeof(float));

      // Copy into multiple kaldi's matrix format
      int cpy_offset = 0;
      for (int feat_idx = 0; feat_idx < feats_row_cnt.size(); feat_idx++) {
        int num_of_rows = feats_row_cnt[feat_idx];
        nnet_out.Resize(num_of_rows, 1706);

        for (MatrixIndexT i = 0; i < num_of_rows; i++) {
          memcpy(nnet_out.Row(i).Data(), (preds + cpy_offset),
                 1706 * sizeof(float));
          cpy_offset += 1706;
        }

        output_list.push_back(nnet_out);
      }
    }

    // check for NaN/inf
    //    for (int32 r = 0; r < nnet_out_host.NumRows(); r++) {
    //      for (int32 c = 0; c < nnet_out_host.NumCols(); c++) {
    //        BaseFloat val = nnet_out_host(r,c);
    //        if (val != val) KALDI_ERR << "NaN in NNet output of : " <<
    //        feature_reader.Key();
    //        if (val == std::numeric_limits<BaseFloat>::infinity())
    //          KALDI_ERR << "inf in NNet coutput of : " <<
    //          feature_reader.Key();
    //      }
    //    }

    // Iterate over feature reader again and write the output
    for (int i = 0; i < feature_keys.size(); i++) {
      KALDI_LOG << "Writing features to file";
      // subtract log-priors from log-posteriors to get quasi-likelihoods
      if (prior_opts.class_frame_counts != "") {
        pdf_prior.SubtractOnLogpost(&output_list[i]);
      }

      nnet_out_host.Resize(output_list[i].NumRows(), output_list[i].NumCols());
      output_list[i].CopyToMat(&nnet_out_host);
      for (MatrixIndexT k = 0; k < nnet_out_host.NumRows(); k++) {
        for (MatrixIndexT j = 0; j < nnet_out_host.NumCols(); j++) {
          // KALDI_LOG << nnet_out_host.Row(k).Data()[j];
        }
      }

      feature_writer.Write(feature_keys[i], nnet_out_host);
    }

    if (num_done == 0) return -1;
    return 0;
  } catch (const std::exception &e) {
    KALDI_ERR << e.what();
    return -1;
  }
}
