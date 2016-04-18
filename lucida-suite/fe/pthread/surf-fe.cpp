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

#include "../../utils/pthreadman.h"
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
using namespace std;

int NTHREADS;

int OVERLAP;

vector<Mat> segs;
FeatureDetector *detector = new SurfFeatureDetector();
int iterations;
vector<vector<KeyPoint> > keys;

vector<Mat> segment(const Mat &img) {
  int height_inc = img.size().height / NTHREADS;
  int width_inc = img.size().width / NTHREADS;

  vector<Mat> segments;

  for (int r = 0; r < img.size().height; r += height_inc) {
    for (int c = 0; c < img.size().width; c += width_inc) {
      int rectoverlap = OVERLAP * 2;
      Rect roi;
      roi.x = c;
      roi.y = r;
      roi.width = (c + width_inc > img.size().width) ? (img.size().width - c)
                                                     : width_inc;
      roi.height = (r + height_inc > img.size().height)
                       ? (img.size().height - r)
                       : height_inc;
      if (r == 0) {  // top row
        if (c != 0)  // top row
          roi.x -= OVERLAP;
        if (c == 0 || c == img.size().width - width_inc)  // corners
          roi.width += OVERLAP;
        else
          roi.width += rectoverlap;
        roi.height += OVERLAP;
      } else if (c == 0) {  // first column
        roi.y -= OVERLAP;
        if (r == img.size().height - height_inc)  // bot left corner
          roi.height += OVERLAP;
        else
          roi.height += rectoverlap;  // any first col segment
        roi.width += OVERLAP;         // cst width
      } else if (r == img.size().height - height_inc) {  // bot row
        roi.x -= OVERLAP;
        roi.y -= OVERLAP;
        if (c == img.size().width - width_inc)  // bot right corner
          roi.width += OVERLAP;
        else
          roi.width += rectoverlap;
        roi.height += OVERLAP;
      } else if (c ==
                 img.size().width -
                     width_inc) {  // last col, corners already accounted for
        roi.x -= OVERLAP;
        roi.y -= OVERLAP;
        roi.width += OVERLAP;
        roi.height += rectoverlap;
      } else {  // any middle image
        roi.x -= OVERLAP;
        roi.y -= OVERLAP;
        roi.width += rectoverlap;
        roi.height += rectoverlap;
      }
      Mat imgroi(img, roi);
      segments.push_back(imgroi);
    }
  }

  return segments;
}

vector<KeyPoint> exec_feature(const Mat &img) {
  vector<KeyPoint> keypoints;
  detector->detect(img, keypoints);

  return keypoints;
}

void *feat_thread(void *tid) {
  int start, *mytid, end;
  mytid = (int *)tid;
  start = (*mytid * iterations);
  end = start + iterations;

  for (int i = start; i < end; ++i) keys[i] = exec_feature(segs[i]);

  return NULL;
}

int main(int argc, char **argv) {
  if (argc < 4) {
    fprintf(stderr, "[ERROR] Invalid arguments provided.\n\n");
    fprintf(stderr, "Usage: %s [NUMBER OF THREADS] [OVERLAP] [INPUT FILE]\n\n",
            argv[0]);
    exit(0);
  }

  STATS_INIT("kernel", "pthread_feature_extraction");
  PRINT_STAT_STRING("abrv", "pthread_fe");

  NTHREADS = atoi(argv[1]);
  OVERLAP = atoi(argv[2]);
  PRINT_STAT_INT("threads", NTHREADS);
  Mat img = imread(argv[3], CV_LOAD_IMAGE_GRAYSCALE);
  if (img.empty()) {
    printf("image not found\n");
    exit(-1);
  }

  int height = img.size().height / NTHREADS;
  int width = img.size().width / NTHREADS;

  PRINT_STAT_INT("rows", img.rows);
  PRINT_STAT_INT("columns", img.cols);
  PRINT_STAT_INT("tile_height", height);
  PRINT_STAT_INT("tile_width", width);
  PRINT_STAT_INT("tile_overlap", OVERLAP);

  tic();
  segs = segment(img);
  PRINT_STAT_DOUBLE("tiling", toc());

  tic();
  int tids[NTHREADS];
  pthread_t threads[NTHREADS];
  pthread_attr_t attr;
  iterations = (segs.size() / NTHREADS);
  keys.resize(segs.size());
  sirius_pthread_attr_init(&attr);
  sirius_pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  // Keys
  for (int i = 0; i < NTHREADS; i++) {
    tids[i] = i;
    sirius_pthread_create(&threads[i], &attr, feat_thread, (void *)&tids[i]);
  }

  for (int i = 0; i < NTHREADS; i++) sirius_pthread_join(threads[i], NULL);

  PRINT_STAT_DOUBLE("pthread_fe", toc());

  STATS_END();

#ifdef TESTING
  int i = 0, r, c;
  CvRect roi;
  Mat output(img.size().height, img.size().width, img.type(), Scalar(0));
  // load color to write color keys
  Mat img2 = imread(argv[3]);
  int height_inc = img.size().height / NTHREADS;
  int width_inc = img.size().width / NTHREADS;

  roi.width = width;
  roi.height = height;
  for (r = 0; r < img.size().height; r += height_inc) {
    for (c = 0; c < img.size().width; c += width_inc) {
      Mat temp = segs[i].clone();
      drawKeypoints(temp, keys[i], temp, CV_RGB(255, 0, 0));
      int rectoverlap = OVERLAP * 2;
      roi.x = c;
      roi.y = r;
      roi.width = (c + width_inc > img.size().width) ? (img.size().width - c)
                                                     : width_inc;
      roi.height = (r + height_inc > img.size().height)
                       ? (img.size().height - r)
                       : height_inc;
      if (r == 0) {  // top row
        if (c != 0)  // top row
          roi.x -= OVERLAP;
        if (c == 0 || c == img.size().width - width_inc)  // corners
          roi.width += OVERLAP;
        else
          roi.width += rectoverlap;
        roi.height += OVERLAP;
      } else if (c == 0) {  // first column
        roi.y -= OVERLAP;
        if (r == img.size().height - height_inc)  // bot left corner
          roi.height += OVERLAP;
        else
          roi.height += rectoverlap;  // any first col segment
        roi.width += OVERLAP;         // cst width
      } else if (r == img.size().height - height_inc) {  // bot row
        roi.x -= OVERLAP;
        roi.y -= OVERLAP;
        if (c == img.size().width - width_inc)  // bot right corner
          roi.width += OVERLAP;
        else
          roi.width += rectoverlap;
        roi.height += OVERLAP;
      } else if (c ==
                 img.size().width -
                     width_inc) {  // last col, corners already accounted for
        roi.x -= OVERLAP;
        roi.y -= OVERLAP;
        roi.width += OVERLAP;
        roi.height += rectoverlap;
      } else {  // any middle image
        roi.x -= OVERLAP;
        roi.y -= OVERLAP;
        roi.width += rectoverlap;
        roi.height += rectoverlap;
      }
      temp.copyTo(img2(roi));
      ++i;
    }
  }

  imwrite("../input/surf-fe.pthread.jpg", img2);
#endif

  // Clean up
  delete detector;

  return 0;
}
