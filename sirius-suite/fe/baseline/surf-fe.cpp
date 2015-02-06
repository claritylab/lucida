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
#include <pthread.h>
#include <time.h>

#include "../../timer/timer.h"
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

  STATS_INIT ("kernel", "feature_extraction");
  PRINT_STAT_STRING ("abrv", "fe");

  // Generate test keys
  Mat img = imread(argv[1], CV_LOAD_IMAGE_GRAYSCALE);
  if (img.empty()) {
    printf("image not found\n");
    exit(-1);
  }

  PRINT_STAT_INT ("rows", img.rows);
  PRINT_STAT_INT ("columns", img.cols);

  tic ();
  vector<KeyPoint> key = exec_feature(img);
  PRINT_STAT_DOUBLE ("fe", toc ());

  STATS_END ();

  // Clean up
  delete detector;

  return 0;
}
