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
 * TODO:
 *
 * @author: Johann Hauswald
 * @contact: jahausw@umich.edu
 */

#include <assert.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <pthread.h>

#include "../../utils/timer.h"
#include "opencv2/core/core.hpp"
#include "opencv2/core/types_c.h"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/nonfree/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/nonfree/gpu.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/stitching/stitcher.hpp"

using namespace cv;
using namespace cv::gpu;
using namespace std;

vector<KeyPoint> exec_feature_gpu_warm(const Mat &img_in) {
  vector<KeyPoint> keypoints;
  gpu::GpuMat img;
  img.upload(img_in);

  gpu::SURF_GPU detector;
  detector(img, gpu::GpuMat(), keypoints);
  return keypoints;
}

vector<KeyPoint> exec_feature_gpu(const Mat &img_in) {
  GpuMat keypoints;
  vector<KeyPoint> keys;
  GpuMat img;
  tic();
  img.upload(img_in);
  PRINT_STAT_DOUBLE("host_to_device_0", toc());

  gpu::SURF_GPU detector;
  tic();
  detector(img, GpuMat(), keypoints);
  PRINT_STAT_DOUBLE("gpu_fe", toc());

  tic();
  detector.downloadKeypoints(keypoints, keys);
  PRINT_STAT_DOUBLE("device_to_host_0", toc());
  return keys;
}

Mat exec_descriptor_gpu(const Mat &img_in, std::vector<KeyPoint> keypoints) {
  GpuMat img;
  tic();
  img.upload(img_in);  // Only 8B grayscale
  PRINT_STAT_DOUBLE("host_to_device_1", toc());
  GpuMat descriptorsGPU;
  Mat descriptors;

  gpu::SURF_GPU extractor;
  tic();
  extractor(img, GpuMat(), keypoints, descriptorsGPU, true);
  PRINT_STAT_DOUBLE("gpu_fd", toc());

  tic();
  descriptorsGPU.download(descriptors);
  PRINT_STAT_DOUBLE("device_to_host_1", toc());

  return descriptors;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "[ERROR] Input file required.\n\n");
    fprintf(stderr, "Usage: %s [INPUT FILE]\n\n", argv[0]);
    exit(0);
  }
  STATS_INIT("kernel", "gpu_feature_description");
  PRINT_STAT_STRING("abrv", "gpu_fd");

  // Generate test keys
  Mat img = imread(argv[1], CV_LOAD_IMAGE_GRAYSCALE);
  if (img.empty()) {
    printf("image not found\n");
    exit(-1);
  }

  PRINT_STAT_INT("rows", img.rows);
  PRINT_STAT_INT("columns", img.cols);

  tic();
  exec_feature_gpu_warm(img);
  PRINT_STAT_DOUBLE("gpu_warm-up", toc());

  vector<KeyPoint> key = exec_feature_gpu(img);

  Mat desc = exec_descriptor_gpu(img, key);

  STATS_END();
#ifdef TESTING
  FILE *f = fopen("../input/surf-fd.gpu", "w");

  fprintf(f, "number of descriptors: %d\n", desc.size().height);

  fclose(f);
#endif

  return 0;
}
