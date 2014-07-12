/* Johann Hauswald
 * jhauswald91@gmail.com
 * 2013
 */

#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include "opencv2/ml/ml.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/core/types_c.h"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/nonfree/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/nonfree/gpu.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "boost/program_options.hpp" 
#include "kp_protobuf.h"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

using namespace cv;
namespace po = boost::program_options;
namespace fs = boost::filesystem;

std::vector<KeyPoint> exec_feature_gpu(const Mat& img_in, const std::string detector_str)
{
	std::vector<KeyPoint> keypoints;
	gpu::GpuMat img; img.upload(img_in); // Only 8B grayscale

	if(detector_str == "FAST"){
		int threshold = 20;
		gpu::FAST_GPU detector(threshold);
		detector(img, gpu::GpuMat(), keypoints);
	}else if(detector_str == "ORB"){
		gpu::ORB_GPU detector;
		detector(img, gpu::GpuMat(), keypoints);
	}else if(detector_str == "SURF"){
		gpu::SURF_GPU detector;
		detector.nOctaves=2; //reduce number of octaves for small image sizes
		detector(img, gpu::GpuMat(), keypoints);
	}else{
		cout << detector_str << "is not a valid GPU Detector" << std::endl;
		assert(0);
	}
	return keypoints;
}

Mat exec_desc_gpu(const Mat& img_in, const std::string extractor_str, 
		std::vector<KeyPoint> keypoints)
{
	gpu::GpuMat img; img.upload(img_in); // Only 8B grayscale
	gpu::GpuMat descriptorsGPU;
	Mat descriptors;

	if(extractor_str == "ORB"){
		gpu::ORB_GPU extractor;
		extractor(img, gpu::GpuMat(), keypoints, descriptorsGPU);
	}else if(extractor_str == "SURF"){
		gpu::SURF_GPU extractor;
		extractor.nOctaves=2; //reduce number of octaves for small image sizes
		extractor(img, gpu::GpuMat(), keypoints, descriptorsGPU, true);
	}else{
		cout << extractor_str << "is not a valid GPU Extractor" << std::endl;
		assert(0);
	}

	descriptorsGPU.download(descriptors);

	return descriptors;
}

