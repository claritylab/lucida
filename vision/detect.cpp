/* Johann Hauswald
 * jahausw@umich.edu
 * 2014
 */

#include <assert.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <fstream>
#include <algorithm>
#include <stdio.h>
#include <tesseract/baseapi.h>
#include <tesseract/strngs.h>
#include "opencv2/core/core.hpp"
#include "opencv2/core/types_c.h"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/nonfree/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/nonfree/gpu.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "boost/program_options.hpp" 
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

using namespace cv;
using namespace std;

namespace po = boost::program_options;
namespace fs = boost::filesystem;

vector<string> split_string(const string& s, vector<string>& elems)
{
	stringstream ss(s);
	string item;
	while (getline(ss, item, ',')) {
		elems.push_back(item);
	}
	return elems;
}

string get_name_from_path(const string& class_path)
{
    fs::path p = fs::system_complete(class_path);
	string filename = p.filename().string();
	return filename;
}

vector<KeyPoint> exec_feature(const Mat& img, FeatureDetector* detector)
{
	vector<KeyPoint> keypoints;
	detector->detect(img, keypoints);

	return keypoints;
}

void exec_text(po::variables_map& vm)
{
    //TODO: add some preprocessing for a bounding box

    tesseract::TessBaseAPI *tess = new tesseract::TessBaseAPI();
    tess->Init(NULL, "eng", tesseract::OEM_DEFAULT);
    tess->SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
    char* outTxt = tess->GetUTF8Text();

    fs::path p = fs::system_complete(vm["text"].as<string>());
    if(fs::is_directory(p))
    {
        fs::directory_iterator end_iter;
        for (fs::directory_iterator dir_itr(p); dir_itr != end_iter; ++dir_itr){
            string img_name(dir_itr->path().string());
            Mat img = imread(img_name, CV_LOAD_IMAGE_GRAYSCALE);
            tess->SetImage(static_cast<uchar *>(img.data), img.size().width,
                                img.size().height, img.channels(), img.step1());
            // tess->SetRectangle();
            char* outTxt = tess->GetUTF8Text();
            cout << "OCR Res: " << outTxt << endl;
        }
    }
    else
    {
        Mat img = imread(vm["text"].as<string>(), CV_LOAD_IMAGE_GRAYSCALE);
        tess->SetImage(static_cast<uchar *>(img.data), img.size().width,
                                img.size().height, img.channels(), img.step1());
        // tess->SetRectangle();
        char* outTxt = tess->GetUTF8Text();
        cout << "OCR Res: " << outTxt << endl;
    }

    // Clean up
    tess->End();
}

Mat exec_desc(const Mat& img, DescriptorExtractor* extractor, vector<KeyPoint> keypoints)
{
	Mat descriptors;

	extractor->compute(img, keypoints, descriptors);

	descriptors.convertTo(descriptors, CV_32F);

	return descriptors;
}

void exec_match(po::variables_map& vm)
{
    assert(vm.count("database"));

    int knn = 1;

    // data
    Mat testImg;
    vector<string> trainImgs;
    Mat testDesc;
    vector<Mat> trainDesc;
    vector< vector<DMatch> > knnMatches;
    vector<int> bestMatches;

    // classes
    FeatureDetector *detector = new SurfFeatureDetector();
    DescriptorExtractor *extractor = new SurfDescriptorExtractor();
    DescriptorMatcher *matcher = new BFMatcher(NORM_L1, false);
    // DescriptorMatcher *matcher = new FlannBasedMatcher();

    // Generate test desc
    testImg = imread(vm["match"].as<string>(), CV_LOAD_IMAGE_GRAYSCALE);
    testDesc = exec_desc(testImg, extractor, exec_feature(testImg, detector));

    // Generate desc
    fs::path p = fs::system_complete(vm["database"].as<string>());
    assert(fs::is_directory(p));

    fs::directory_iterator end_iter;
    for (fs::directory_iterator dir_itr(p); dir_itr != end_iter; ++dir_itr){
        string img_name(dir_itr->path().string());
        Mat img = imread(img_name, CV_LOAD_IMAGE_GRAYSCALE);
        trainDesc.push_back(exec_desc(img, extractor, exec_feature(img, detector)));
        trainImgs.push_back(img_name);
        int temp = 0;
        bestMatches.push_back(temp);
    }
    
    // Match
    matcher->add(trainDesc);
    matcher->train();
    matcher->knnMatch(testDesc, knnMatches, knn);

    // Filter results
    for(vector< vector<DMatch> >::const_iterator it = knnMatches.begin(); it != knnMatches.end(); ++it){
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

    cout << "Best match: " << trainImgs[bestIdx] << " Score: " << bestScore << endl;

    // Clean up
    delete detector;
    delete extractor;
    delete matcher;
}

void exec_hog(po::variables_map& vm)
{
    if(vm["verbose"].as<int>() > 0)
        cout << "Exec HoG..." << endl;

    Mat img;
#ifdef USE_GPU
    gpu::GpuMat gpu_img;
    gpu::HOGDescriptor hog;
    hog.setSVMDetector(gpu::HOGDescriptor::getDefaultPeopleDetector());
#else
    HOGDescriptor hog;
    hog.setSVMDetector(HOGDescriptor::getDefaultPeopleDetector());
#endif

	if(vm.count("stream")){
		string input_str = vm["stream"].as<string>();
        vector<string> split_names;
		split_string(input_str, split_names);
		for(int i = 0; i < split_names.size(); ++i){
			string& class_path = split_names[i];
			string class_name = get_name_from_path(class_path);
			fs::path p = fs::system_complete(class_path);
			assert(fs::is_directory(p));

			fs::directory_iterator end_iter;
			for (fs::directory_iterator dir_itr(p); dir_itr != end_iter; ++dir_itr){
				string img_name(dir_itr->path().string());
                img = imread(img_name, CV_LOAD_IMAGE_GRAYSCALE);
                vector<cv::Rect> found;
#ifdef USE_GPU
                gpu_img.upload(img);
                hog.detectMultiScale(gpu_img, found);
#else
                hog.detectMultiScale(img, found);
#endif
                if(vm["verbose"].as<int>() > 1)
                    cout << "Processing " << img_name << "..." << endl;
            }
        }
    }
    else if(vm.count("image")){
        img = imread(vm["image"].as<string>(), CV_LOAD_IMAGE_GRAYSCALE);
        vector<cv::Rect> found;
#ifdef USE_GPU
        gpu_img.upload(img);
        hog.detectMultiScale(gpu_img, found);
#else
        hog.detectMultiScale(img, found);
#endif
        if(vm["verbose"].as<int>() > 1)
            cout << "Processed " << vm["image"].as<string>() << "..." << endl;
    }

    if(vm["verbose"].as<int>() > 0)
        cout << "Done HoG." << endl;
}

po::variables_map parse_opts( int ac, char** av )
{
	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "Produce help message")
		("match,m", po::value<string>(), "Test Image for match in database (image)")
		("text,t", po::value<string>(), "Detect text in image(s) (directory or image)")
		("database,d", po::value<string>(), "Database to test for match (directory)")
		("walk,k", po::value<bool>()->default_value(false), "Stream images to a HOG person detector")
		("stream,s", po::value<string>(), "Folders with image stream separated by a comma (walk)")
		("image,e", po::value<string>(), "Input Image (walk)")

		("gpu,u", po::value<bool>()->default_value(false), "Use GPU? Only for specific algorithms") 

		("verbose,v", po::value<int>()->default_value(0), "Debug levels: 0: no info, 1: pipeline stages, 2: all") 
		;

	po::variables_map vm;
	po::store(po::parse_command_line(ac, av, desc), vm);
	po::notify(vm);    

	if (vm.count("help")) {
		cout << desc << "\n";
		return vm;
	}
	return vm;
}

int main( int argc, char** argv )
{
	po::variables_map vm = parse_opts(argc, argv);
    
	if(vm["walk"].as<bool>()){
		exec_hog(vm);
    }else if(vm.count("match")){
        exec_match(vm);
    }else if(vm.count("text")){
        exec_text(vm);
	}else{
		cout << "For help: " << argv[0] << " --help" << endl;
	}

	return 0;
}
