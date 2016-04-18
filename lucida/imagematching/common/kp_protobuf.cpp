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

#include "kp_protobuf.h"

using namespace cv;
using namespace std;

void save_map(map<string, Mat> inputMap, const char *filename) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  store_mat::Map store_map;
  store_mat::Pair *p;
  store_mat::mat_data m;

  store_map.set_size(inputMap.size());

  map<string, Mat>::iterator it;
  for (it = inputMap.begin(); it != inputMap.end(); ++it) {
    p = store_map.add_pair();
    p->set_str((*it).first);
    m.set_row((*it).second.rows);
    m.set_col((*it).second.cols);
    m.set_flags((*it).second.flags);
    m.set_step((*it).second.step);
    m.set_size((*it).second.dataend - (*it).second.datastart);
    m.set_type((*it).second.type());
    for (int i = 0; i < (*it).second.dataend - (*it).second.datastart; i++) {
      m.add_data((*it).second.at<uchar>(i));
    }

    p->mutable_mat()->CopyFrom(m);
  }

  if (!store_map.IsInitialized()) {
    cerr << "Not properly initialized\n";
  }

  fstream output(filename, ios::out | ios::binary);
  store_map.SerializeToOstream(&output);

  google::protobuf::ShutdownProtobufLibrary();

  return;
}

map<string, Mat> read_map(const char *filename) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  map<string, Mat> m;
  store_mat::Map read_stored_data;
  store_mat::mat_data read_stored_mat;
  fstream input(filename, ios::in | ios::binary);
  read_stored_data.ParseFromIstream(&input);

  Mat stored_mat;
  string stored_str;

  for (int i = 0; i < read_stored_data.size(); i++) {
    stored_str = read_stored_data.pair(i).str();
    read_stored_mat = read_stored_data.pair(i).mat();
    stored_mat.create(read_stored_mat.row(), read_stored_mat.col(),
                      read_stored_mat.type());
    stored_mat.rows = read_stored_mat.row();
    stored_mat.cols = read_stored_mat.col();
    stored_mat.flags = read_stored_mat.flags();
    stored_mat.step = read_stored_mat.step();

    for (int j = 0; j < read_stored_mat.size(); j++) {
      stored_mat.at<uchar>(j) = (uchar)read_stored_mat.data(j);
    }

    m.insert(pair<string, Mat>(stored_str, stored_mat));
  }

  google::protobuf::ShutdownProtobufLibrary();

  return m;
}

std::vector<KeyPoint> read_keypoints(const char *filename) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  kp::all_kp read_stored_data;
  std::vector<KeyPoint> keypoints;
  fstream input(filename, ios::in | ios::binary);
  read_stored_data.ParseFromIstream(&input);

  KeyPoint kp;

  for (int i = 0; i < read_stored_data.size(); i++) {
    kp.pt.x = read_stored_data.keypoint(i).x();
    kp.pt.y = read_stored_data.keypoint(i).y();
    kp.size = read_stored_data.keypoint(i).size();
    kp.angle = read_stored_data.keypoint(i).angle();
    kp.response = read_stored_data.keypoint(i).response();
    kp.octave = read_stored_data.keypoint(i).octave();
    kp.class_id = read_stored_data.keypoint(i).class_id();
    keypoints.push_back(kp);
  }

  google::protobuf::ShutdownProtobufLibrary();

  return keypoints;
}

void save_keypoints(std::vector<KeyPoint> keypoints, const char *filename) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  kp::all_kp store_keypoints;
  kp::Keypoint *kp;
  store_keypoints.set_size(keypoints.size());

  for (int i = 0; i < keypoints.size(); i++) {
    kp = store_keypoints.add_keypoint();
    kp->set_x(keypoints.at(i).pt.x);
    kp->set_y(keypoints.at(i).pt.y);
    kp->set_size(keypoints.at(i).size);
    kp->set_angle(keypoints.at(i).angle);
    kp->set_response(keypoints.at(i).response);
    kp->set_octave(keypoints.at(i).octave);
    kp->set_class_id(keypoints.at(i).class_id);
  }

  if (!store_keypoints.IsInitialized()) {
    cerr << "Not properly initialized\n";
  }

  fstream output(filename, ios::out | ios::binary);
  store_keypoints.SerializeToOstream(&output);

  google::protobuf::ShutdownProtobufLibrary();

  return;
}

void save_mat(Mat descriptors, const char *filename) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  store_mat::mat_data store_mat;
  store_mat.set_row(descriptors.rows);
  store_mat.set_col(descriptors.cols);
  store_mat.set_flags(descriptors.flags);
  store_mat.set_step(descriptors.step);
  store_mat.set_size(descriptors.dataend - descriptors.datastart);
  store_mat.set_type(descriptors.type());

  for (int i = 0; i < descriptors.dataend - descriptors.datastart; i++) {
    store_mat.add_data(descriptors.at<uchar>(i));
  }

  if (!store_mat.IsInitialized()) {
    cerr << "Not properly initialized\n";
  }

  fstream output(filename, ios::out | ios::binary);
  store_mat.SerializeToOstream(&output);

  google::protobuf::ShutdownProtobufLibrary();

  return;
}

Mat read_mat(const char *filename) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  store_mat::mat_data read_stored_data;
  fstream input(filename, ios::in | ios::binary);
  read_stored_data.ParseFromIstream(&input);

  Mat descriptors(read_stored_data.row(), read_stored_data.col(),
                  read_stored_data.type());

  descriptors.rows = read_stored_data.row();
  descriptors.cols = read_stored_data.col();
  descriptors.flags = read_stored_data.flags();
  descriptors.step = read_stored_data.step();

  for (int i = 0; i < read_stored_data.size(); i++) {
    descriptors.at<uchar>(i) = (uchar)read_stored_data.data(i);
  }

  google::protobuf::ShutdownProtobufLibrary();

  return descriptors;
}
