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

#include "detect.h"
#include "kp_protobuf.h"

// using namespace cv;
// using namespace std;

namespace po = boost::program_options;
namespace fs = boost::filesystem;

struct timeval tv1, tv2;
int debug = 0;

string make_pbdesc(string img_name) {
  string res;
  size_t pos = img_name.find(".jpg");
  if (pos != std::string::npos) res = img_name.substr(0, pos);
  res = res + ".pb";
  return res;
}

vector<KeyPoint> exec_feature_gpu(const Mat &img_in,
                                  const string &detector_str) {
  vector<KeyPoint> keypoints;
  gpu::GpuMat img;
  img.upload(img_in);  // Only 8B grayscale

  if (detector_str == "FAST") {
    int threshold = 20;
    gpu::FAST_GPU detector(threshold);
    detector(img, gpu::GpuMat(), keypoints);
  } else if (detector_str == "ORB") {
    gpu::ORB_GPU detector;
    detector(img, gpu::GpuMat(), keypoints);
  } else if (detector_str == "SURF") {
    gpu::SURF_GPU detector;
    detector.nOctaves = 2;  // reduce number of octaves for small image sizes
    detector(img, gpu::GpuMat(), keypoints);
  } else {
    cout << detector_str << "is not a valid GPU Detector" << endl;
    assert(0);
  }
  return keypoints;
}

vector<KeyPoint> exec_feature(const Mat &img, FeatureDetector *detector) {
  vector<KeyPoint> keypoints;
  detector->detect(img, keypoints);

  return keypoints;
}

void exec_text(po::variables_map &vm) {
  // TODO: add some preprocessing for a bounding box

  tesseract::TessBaseAPI *tess = new tesseract::TessBaseAPI();
  tess->Init(NULL, "eng", tesseract::OEM_DEFAULT);
  tess->SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);

  fs::path p = fs::system_complete(vm["text"].as<string>());
  if (fs::is_directory(p)) {
    fs::directory_iterator end_iter;
    for (fs::directory_iterator dir_itr(p); dir_itr != end_iter; ++dir_itr) {
      string img_name(dir_itr->path().string());
      Mat img = imread(img_name, CV_LOAD_IMAGE_GRAYSCALE);
      tess->SetImage(static_cast<uchar *>(img.data), img.size().width,
                     img.size().height, img.channels(), img.step1());
      // tess->SetRectangle();
      char *outTxt = tess->GetUTF8Text();
      // cout << "OCR Res: " << outTxt << endl;
    }
  } else {
    Mat img = imread(vm["text"].as<string>(), CV_LOAD_IMAGE_GRAYSCALE);
    tess->SetImage(static_cast<uchar *>(img.data), img.size().width,
                   img.size().height, img.channels(), img.step1());
    char *outTxt = tess->GetUTF8Text();
    // cout << "OCR Res: " << outTxt << endl;
  }

  // Clean up
  tess->End();
}

Mat exec_desc_gpu(const Mat &img_in, const string &extractor_str,
                  vector<KeyPoint> keypoints) {
  gpu::GpuMat img;
  img.upload(img_in);  // Only 8B grayscale
  gpu::GpuMat descriptorsGPU;
  Mat descriptors;

  if (extractor_str == "ORB") {
    gpu::ORB_GPU extractor;
    extractor(img, gpu::GpuMat(), keypoints, descriptorsGPU);
  } else if (extractor_str == "SURF") {
    gpu::SURF_GPU extractor;
    extractor.nOctaves = 2;  // reduce number of octaves for small image sizes
    extractor(img, gpu::GpuMat(), keypoints, descriptorsGPU, true);
  } else {
    cout << extractor_str << "is not a valid GPU Extractor" << endl;
    assert(0);
  }

  descriptorsGPU.download(descriptors);

  return descriptors;
}

Mat exec_desc(const Mat &img, DescriptorExtractor *extractor,
              vector<KeyPoint> keypoints) {
  Mat descriptors;

  extractor->compute(img, keypoints, descriptors);

  descriptors.convertTo(descriptors, CV_32F);

  return descriptors;
}

void exec_match(po::variables_map &vm) {
  cout << "Matching image..." << endl;
  assert(vm.count("database"));
  assert(vm.count("debug"));
  debug = vm["debug"].as<int>();

  int knn = 1;
  int gpu = 0;

  // data
  Mat testImg;
  vector<string> trainImgs;
  Mat testDesc;
  vector<Mat> trainDesc;
  vector<vector<DMatch> > knnMatches;
  vector<int> bestMatches;
  unsigned int runtimefeat = 0, totalfeat = 0;
  unsigned int runtimedesc = 0, totaldesc = 0;
  unsigned int runtimecluster = 0;
  unsigned int runtimesearch = 0;
  unsigned int totaltime = 0;
  struct timeval tot1, tot2;
  // int numimgs = 0;

  gettimeofday(&tot1, NULL);
  // classes
  FeatureDetector *detector = new SurfFeatureDetector();
  DescriptorExtractor *extractor = new SurfDescriptorExtractor();
  // DescriptorMatcher *matcher = new BFMatcher(NORM_L1, false); //KNN
  DescriptorMatcher *matcher = new FlannBasedMatcher();  // ANN

  // Generate test keys
  testImg = imread(vm["match"].as<string>(), CV_LOAD_IMAGE_GRAYSCALE);
  gettimeofday(&tv1, NULL);
  vector<KeyPoint> keys = exec_feature(testImg, detector);
  gettimeofday(&tv2, NULL);
  runtimefeat =
      (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);

  // Generate test desc
  gettimeofday(&tv1, NULL);
  testDesc = exec_desc(testImg, extractor, keys);
  gettimeofday(&tv2, NULL);
  runtimedesc =
      (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);

  // just use feat + desc of input image
  totalfeat += runtimefeat;
  totaldesc += runtimedesc;

  // Generate desc
  fs::path p = fs::system_complete(vm["database"].as<string>());
  assert(fs::is_directory(p));

  fs::directory_iterator end_iter;
  for (fs::directory_iterator dir_itr(p); dir_itr != end_iter; ++dir_itr) {
    string img_name(dir_itr->path().string());
    Mat img = imread(img_name, CV_LOAD_IMAGE_GRAYSCALE);
    string pb_desc = make_pbdesc(img_name);
    Mat desc;
    if (fs::is_regular_file(pb_desc))
      desc = read_mat(pb_desc.c_str());
    else {
      gettimeofday(&tv1, NULL);
      keys =
          (gpu) ? exec_feature_gpu(img, "SURF") : exec_feature(img, detector);
      gettimeofday(&tv2, NULL);
      runtimefeat =
          (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);

      gettimeofday(&tv1, NULL);
      desc = (gpu) ? exec_desc_gpu(img, "SURF", keys)
                   : exec_desc(img, extractor, keys);
      // desc.convertTo(desc, CV_32F);
      gettimeofday(&tv2, NULL);
      runtimedesc =
          (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
    }

    trainDesc.push_back(desc);

    trainImgs.push_back(img_name);
    int temp = 0;
    bestMatches.push_back(temp);
    if (debug > 1)
      cout << "Feature: " << (double)runtimefeat / 1000000
           << " Descriptor: " << (double)runtimedesc / 1000000 << endl;
    // ++numimgs;
    // totalfeat += runtimefeat;
    // totaldesc += runtimedesc;
  }

  // Cluster
  gettimeofday(&tv1, NULL);
  matcher->add(trainDesc);
  matcher->train();
  gettimeofday(&tv2, NULL);
  runtimecluster =
      (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);

  // Match
  gettimeofday(&tv1, NULL);
  matcher->knnMatch(testDesc, knnMatches, knn);
  gettimeofday(&tv2, NULL);
  runtimesearch =
      (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);

  // Filter results
  for (vector<vector<DMatch> >::const_iterator it = knnMatches.begin();
       it != knnMatches.end(); ++it) {
    for (vector<DMatch>::const_iterator it2 = it->begin(); it2 != it->end();
         ++it2) {
      ++bestMatches[(*it2).imgIdx];
    }
  }

  // Find best match
  int bestScore = 0;
  int bestIdx = -1;
  for (int i = 0; i < bestMatches.size(); ++i) {
    if (bestMatches[i] >= bestScore) {
      bestScore = bestMatches[i];
      bestIdx = i;
    }
  }

  if (debug > 0)
    cout << "Best match: " << trainImgs[bestIdx] << " Score: " << bestScore
         << endl;
  cout << "Found match" << endl;

  // Clean up
  delete detector;
  delete extractor;
  delete matcher;
  // gettimeofday(&tot2,NULL);
  // totaltime = (tot2.tv_sec-tot1.tv_sec)*1000000 +
  // (tot2.tv_usec-tot1.tv_usec);

  totaltime = totalfeat + totaldesc + runtimecluster + runtimesearch;
  if (debug > 0) {
    printf(" Feat time: %.2f ms (%.2f)\n ", (double)totalfeat / 1000,
           (double)totalfeat / totaltime);
    printf(" Desc time: %.2f ms (%.2f)\n ", (double)totaldesc / 1000,
           (double)totaldesc / totaltime);
    printf(" Cluster time: %.2f ms (%.2f)\n ", (double)runtimecluster / 1000,
           (double)runtimecluster / totaltime);
    printf(" Search time: %.2f ms (%.2f)\n ", (double)runtimesearch / 1000,
           (double)runtimesearch / totaltime);
  }
}

void build_db(po::variables_map &vm) {
  int gpu = 0;
  // data
  vector<string> trainImgs;
  vector<Mat> trainDesc;
  FeatureDetector *detector = new SurfFeatureDetector();
  DescriptorExtractor *extractor = new SurfDescriptorExtractor();
  // DescriptorMatcher *matcher = new BFMatcher(NORM_L1, false); //KNN
  DescriptorMatcher *matcher = new FlannBasedMatcher();  // ANN

  // Generate desc
  fs::path p = fs::system_complete(vm["build"].as<string>());
  assert(fs::is_directory(p));

  fs::directory_iterator end_iter;
  for (fs::directory_iterator dir_itr(p); dir_itr != end_iter; ++dir_itr) {
    string img_name(dir_itr->path().string());
    Mat img = imread(img_name, CV_LOAD_IMAGE_GRAYSCALE);
    vector<KeyPoint> keys =
        (gpu) ? exec_feature_gpu(img, "SURF") : exec_feature(img, detector);
    Mat desc = (gpu) ? exec_desc_gpu(img, "SURF", keys)
                     : exec_desc(img, extractor, keys);
    trainDesc.push_back(desc);
    trainImgs.push_back(img_name);
    save_mat(desc, make_pbdesc(img_name).c_str());
  }

  // Cluster
  matcher->add(trainDesc);
  matcher->train();

  // Clean up
  delete detector;
  delete extractor;
  delete matcher;
}

void build_model(DescriptorMatcher *matcher, vector<string> *trainImgs){
	// vector<string> trainImgs;
	vector<Mat> trainDesc;
	FeatureDetector *detector = new SurfFeatureDetector();
	DescriptorExtractor *extractor = new SurfDescriptorExtractor();
	
  // Generate desc
  string db = fs::current_path().parent_path().string() + "/common/matching/landmarks/db";
	fs::path p = fs::system_complete(db);
	assert(fs::is_directory(p));

	fs::directory_iterator end_iter;
	for(fs::directory_iterator dir_itr(p); dir_itr != end_iter; ++dir_itr){
		string img_name(dir_itr->path().string());
		Mat img = imread(img_name, CV_LOAD_IMAGE_GRAYSCALE);
		vector<KeyPoint> keys = exec_feature(img, detector);
		Mat desc = exec_desc(img, extractor, keys);
		trainDesc.push_back(desc);
		trainImgs->push_back(img_name);
		save_mat(desc, make_pbdesc(img_name).c_str());
	}
	
	// Cluster
	matcher->add(trainDesc);
	matcher->train();
	
	// Clean up
	delete detector;
	delete extractor;
}

string exec_match(string img_path, DescriptorMatcher *matcher, vector<string> *trainImgs){
  cout << "Matching image..." << endl;
	// data
	int debug = 0;
	int analyze = 0;
	Mat testImg;
	Mat testDesc;
	vector<vector<DMatch> > knnMatches;
	vector<int> bestMatches(trainImgs->size(), 0);
	
	unsigned int runtimefeat = 0, totalfeat = 0;
	unsigned int runtimedesc = 0, totaldesc = 0;
	unsigned int runtimecluster = 0;
	unsigned int runtimesearch = 0;
	unsigned int totaltime = 0;
	// struct timeval tot1, tot2;
	struct timeval tv1, tv2;

	// gettimeofday(&tot1, NULL);
	// classes
	FeatureDetector *detector = new SurfFeatureDetector();
	DescriptorExtractor *extractor = new SurfDescriptorExtractor();

	// Generate test keys
	testImg = imread(img_path, CV_LOAD_IMAGE_GRAYSCALE);
	gettimeofday(&tv1, NULL);
	vector<KeyPoint> keys = exec_feature(testImg, detector);
	gettimeofday(&tv2, NULL);
	runtimefeat = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);

	// Generate test desc
	gettimeofday(&tv1, NULL);
	testDesc = exec_desc(testImg, extractor, keys);
	gettimeofday(&tv2, NULL);
	runtimedesc = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);

	// just use feat + desc of input image
	totalfeat += runtimefeat;
	totaldesc += runtimedesc;

	gettimeofday(&tv1, NULL);
	// Match
	// FIXME maybe we can replace the knnMatch method with match, which by default
	// returns the best match for each descriptor
	int knn = 1;
	matcher->knnMatch(testDesc, knnMatches, knn);
	
	gettimeofday(&tv2, NULL);
	runtimesearch = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);

	// Filter results
	for(vector<vector<DMatch> >::const_iterator it = knnMatches.begin(); it != knnMatches.end(); ++it){
		for(vector<DMatch>::const_iterator it2 = it->begin(); it2 != it->end(); ++it2){
			++bestMatches[(*it2).imgIdx];
		}
	}

	// Find best match
	int bestScore = 0;
	int bestIdx = -1;
	for(int i = 0; i < bestMatches.size(); ++i){
		if(bestMatches[i] >= bestScore){
			bestScore = bestMatches[i];
			bestIdx = i;
		}
	}

	if (debug > 0)
		cout << "Best match: " << trainImgs->at(bestIdx) << " Score: " << bestScore << endl;
	cout << "Found match" << endl;

  	// Clean up
	delete detector;
	delete extractor;

	//totaltime = totalfeat + totaldesc + runtimecluster + runtimesearch;
	totaltime = totalfeat + totaldesc + runtimesearch;
	if (debug > 0) {
		printf(" Feat time: %.2f ms (%.2f)\n ", (double)totalfeat / 1000,
			(double)totalfeat / totaltime);
		printf(" Desc time: %.2f ms (%.2f)\n ", (double)totaldesc / 1000,
			(double)totaldesc / totaltime);
		printf(" Search time: %.2f ms (%.2f)\n ", (double)runtimesearch / 1000,
			(double)runtimesearch / totaltime);
	}
	if(analyze){
		printf("%.2f,%.2f,%.2f\n", (double)totalfeat / 1000, (double)totaldesc / 1000, (double)runtimesearch / 1000);
	}
	return trainImgs->at(bestIdx);
}

