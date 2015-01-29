/* Johann Hauswald
 * jahausw@umich.edu
 * 2014
 */

#include <assert.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
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
#define OVERLAP 0

vector<Mat> segs;
vector<vector<KeyPoint> > keys;
FeatureDetector *detector = new SurfFeatureDetector();
DescriptorExtractor *extractor = new SurfDescriptorExtractor();
int iterations;

float calculateMiliseconds(timeval t1, timeval t2) {
  float elapsedTime;
  elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
  elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
  return elapsedTime;
}

vector<KeyPoint> exec_feature(const Mat &img) {
  vector<KeyPoint> keypoints;
  detector->detect(img, keypoints);

  return keypoints;
}

Mat exec_desc(const Mat &img, vector<KeyPoint> keypoints) {
  Mat descriptors;

  extractor->compute(img, keypoints, descriptors);

  descriptors.convertTo(descriptors, CV_32F);

  return descriptors;
}

void *desc_thread(void *tid) {
  int start, *mytid, end;
  mytid = (int *)tid;
  start = (*mytid * iterations);
  end = start + iterations;

  // printf ("Thread %d doing iterations %d to %d\n", *mytid, start, end-1);

  for (int i = start; i < end; ++i)
      Mat testDesc = exec_desc(segs[i], keys[i]);
}

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

int main(int argc, char **argv) {
  if(argc < 3){
      printf("%s <threads> <input>\n", argv[0]);
      exit(0);
  }
  // data
  float runtimecut = 0;
  float runtimefeat = 0;
  float runtimedescseq = 0;
  float runtimedescpar = 0;
  struct timeval t1, t2;

  NTHREADS = atoi(argv[1]);
  // Generate test keys
  Mat img = imread(argv[2], CV_LOAD_IMAGE_GRAYSCALE);
  if (img.empty()) {
    printf("image not found\n");
    exit(-1);
  }

  int height = img.size().height / NTHREADS;
  int width = img.size().width / NTHREADS;
  printf("Threads= %d, block H=%d, block W=%d\n", NTHREADS, height, width);

  gettimeofday(&t1, NULL);
  segs = segment(img);
  gettimeofday(&t2, NULL);
  runtimecut = calculateMiliseconds(t1, t2);

  gettimeofday(&t1, NULL);
  for (int i = 0; i < segs.size(); ++i) {
    keys.push_back(exec_feature(segs[i]));
  }
  gettimeofday(&t2, NULL);
  runtimefeat = calculateMiliseconds(t1, t2);

  gettimeofday(&t1, NULL);
  for (int i = 0; i < segs.size(); ++i)
      Mat testDesc = exec_desc(segs[i], keys[i]);
  gettimeofday(&t2, NULL);
  runtimedescseq = calculateMiliseconds(t1, t2);

  gettimeofday(&t1, NULL);
  int start, tids[NTHREADS];
  pthread_t threads[NTHREADS];
  pthread_attr_t attr;
  iterations = (segs.size() / NTHREADS);
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  for (int i = 0; i < NTHREADS; i++) {
    tids[i] = i;
    pthread_create(&threads[i], &attr, desc_thread, (void *)&tids[i]);
  }

  for (int i = 0; i < NTHREADS; i++)
      pthread_join(threads[i], NULL);

  gettimeofday(&t2, NULL);
  runtimedescpar = calculateMiliseconds(t1, t2);

  // Clean up
  delete detector;
  delete extractor;

  printf("SURF CUT Time=%4.3f ms\n", runtimecut);
  printf("SURF FE CPU Time=%4.3f ms\n", runtimefeat);
  printf("SURF FD CPU Time=%4.3f ms\n", runtimedescseq);
  printf("SURF FD PThread CPU Time=%4.3f ms\n", runtimedescpar);
  printf("Speedup=%4.3f\n", (float)runtimedescseq/(float)runtimedescpar);

  return 0;
}
