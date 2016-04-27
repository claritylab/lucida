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

#ifndef KP_PROTOBUF_H
#define KP_PROTOBUF_H

#include <iostream>
#include <fstream>
#include <stdio.h>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/nonfree/features2d.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "keypoints.pb.h"
#include "store_mat.pb.h"

using namespace cv;
using namespace std;

void save_map(map<string, Mat> inputMap, const char *filename);
map<string, Mat> read_map(const char *filename);
void save_keypoints(std::vector<KeyPoint> keypoints, const char *filename);
std::vector<KeyPoint> read_keypoints(const char *filename);
void save_mat(Mat descriptors, const char *filename);
Mat read_mat(const char *filename);

#endif /*KP_PROTOBUF_H*/
