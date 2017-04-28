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
 *
 * @author: Johann Hauswald
 * @contact: jahausw@umich.edu
 */

/**
 * Edit to be integrated into thrift framework
 * 
 * @author: Hailong Yang
 * @contact: hailong@umich.edu
 */

#include <algorithm>
#include "Image.h"
#include "IMMHandler.h"

using namespace cv;
using namespace std;
namespace po = boost::program_options;
namespace fs = boost::filesystem;

string Image::saveToFS(const string &data) {
   struct timeval tp;
   gettimeofday(&tp, NULL);
   long int timestamp = tp.tv_sec * 1000 + tp.tv_usec / 1000;
   string image_path = "input-" + to_string(timestamp) + ".jpg";
   ofstream image_file(image_path, ios::binary);
   print("Size of image written to FS: " << data.size());
   image_file.write(data.c_str(), data.size());
   image_file.close();
   return image_path;
}

vector<float> Image::matToVector(unique_ptr<Mat> mat) {
   std::vector<float> array;
   if (mat->isContinuous()) {
      array.assign((float*)mat->datastart, (float*)mat->dataend);
   } else {
      for (int i = 0; i < mat->rows; ++i) {
         array.insert(array.end(),
               (float*)mat->ptr<uchar>(i),
               (float*)mat->ptr<uchar>(i) + mat->cols);
      }
   }
   return array;
}

unique_ptr<Mat> Image::imageToMatObj(const string &data) {
   // Logging: save the image to file system.
   string image_path = saveToFS(data);
   // Load the image and extract features into a matrix.
   Mat img = imread(image_path, CV_LOAD_IMAGE_GRAYSCALE);
   vector<KeyPoint> keys;
   unique_ptr<SurfFeatureDetector>(new SurfFeatureDetector())->
         detect(img, keys);
   unique_ptr<Mat> desc(new Mat());
   unique_ptr<SurfDescriptorExtractor>(new SurfDescriptorExtractor())->
         compute(img, keys, *desc);
   desc->convertTo(*desc, OPENCV_TYPE);
   return desc;
}

const string Image::imageToMatString(const string &data) {
   // Convert image data to Mat.
   unique_ptr<Mat> desc = imageToMatObj(data);
   // Convert Mat to CSV string.
   string rtn;
   for (int i = 0; i < desc->rows; ++i) {
      for (int j = 0; j < desc->cols; ++j) {
         rtn += to_string(*((float*) desc->ptr<uchar>(i) + j)); // CV_32F
         rtn += ",";
      }
      rtn.pop_back();
      rtn += '\n';
   }
   return rtn;
}

unique_ptr<Mat> Image::matStringToMatObj(const string &mat) {
   unique_ptr<Mat> rtn(new Mat());
   stringstream ss(mat);
   for (string line; getline(ss, line); ) {
      vector<double> dvals;
      stringstream ssline(line);
      for (string val; getline(ssline, val, ','); )  {
         dvals.push_back(stod(val));
      }
      Mat mline(dvals, true);
      transpose(mline, mline);
      rtn->push_back(mline);
   }
   int ch = CV_MAT_CN(OPENCV_TYPE);
   *rtn = rtn->reshape(ch);
   rtn->convertTo(*rtn, OPENCV_TYPE);
   return rtn;
}

int Image::match(
      vector<unique_ptr<StoredImage>> &train_images,
      unique_ptr<QueryImage> query_image) {
   if (train_images.empty()) {
      throw runtime_error("Error! No images!");
   }
   // Train a FlannBasedMatcher.
   unique_ptr<DescriptorMatcher> matcher(new FlannBasedMatcher());
   vector<Mat> train_mats;
   for (unique_ptr<StoredImage> &image_ptr : train_images) {
       if(image_ptr->desc->type() != OPENCV_TYPE) {
           image_ptr->desc->convertTo(*(image_ptr->desc), OPENCV_TYPE);
       }
       train_mats.push_back(*(image_ptr->desc));
   }
   matcher->add(train_mats);
   matcher->train();
   // Prepare to match.
   vector<vector<DMatch>> knn_matches;
   vector<int> scores(train_images.size(), 0);
   int knn = 1;
   // Match.
   matcher->knnMatch(*(query_image->desc), knn_matches, knn);
   // Filter results.
   for (auto &v : knn_matches) {
      for(auto &dMatch : v){
         ++scores[dMatch.imgIdx];
      }
   }
   // Find the best match.
   auto best = max_element(scores.begin(), scores.end());
   return best - scores.begin();
}

bool Image::matEqual(unique_ptr<Mat> a, unique_ptr<Mat> b) {
   return matToVector(move(a)) == matToVector(move(b));
}
