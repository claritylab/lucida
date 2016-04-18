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
#include <time.h>

#include "../../utils/timer.h"
#include "opencv2/core/core.hpp"
#include "opencv2/core/types_c.h"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/nonfree/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/nonfree/gpu.hpp"
#include "opencv2/objdetect/objdetect.hpp"

using namespace cv;
using namespace std;

vector<vector<KeyPoint> > keys;
FeatureDetector *detector = new SurfFeatureDetector();
int iterations;

vector<KeyPoint> exec_feature(const Mat &img) {
  vector<KeyPoint> keypoints;
  detector->detect(img, keypoints);

  return keypoints;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "[ERROR] Input file required.\n\n");
    fprintf(stderr, "Usage: %s [INPUT FILE]\n\n", argv[0]);
    exit(0);
  }

  STATS_INIT("kernel", "feature_extraction");
  PRINT_STAT_STRING("abrv", "fe");

  // Generate test keys
  Mat img = imread(argv[1], CV_LOAD_IMAGE_GRAYSCALE);
  if (img.empty()) {
    printf("image not found\n");
    exit(-1);
  }

  PRINT_STAT_INT("rows", img.rows);
  PRINT_STAT_INT("columns", img.cols);

  tic();
  vector<KeyPoint> key = exec_feature(img);
  PRINT_STAT_DOUBLE("fe", toc());

  STATS_END();

#ifdef TESTING
  Mat output;

  drawKeypoints(img, key, output, CV_RGB(255, 0, 0));
  imwrite("../input/surf-fe.baseline.jpg", output);
#endif

  // Clean up
  delete detector;

  return 0;
}
