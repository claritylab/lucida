/* Johann Hauswald
 * jhauswald91@gmail.com
 * 2013
 */

#include <assert.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <fstream>
#include <stdio.h>
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

int gpu = 0;

std::vector<std::string> split_string(const std::string& s, std::vector<std::string>& elems)
{
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, ',')) {
		elems.push_back(item);
	}
	return elems;
}

std::string get_name_from_path(const std::string& class_path)
{
	fs::path p = fs::system_complete(class_path);
	std::string filename = p.filename().string();
	return filename;
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
		string input_str = vm["stream"].as<std::string>();
        vector<string> split_names;
		split_string(input_str, split_names);
		for(int i = 0; i < split_names.size(); ++i){
			std::string& class_path = split_names[i];
			std::string class_name = get_name_from_path(class_path);
			fs::path p = fs::system_complete(class_path);
			assert(fs::is_directory(p));

			fs::directory_iterator end_iter;
			for (fs::directory_iterator dir_itr(p); dir_itr != end_iter; ++dir_itr){
				std::string img_name(dir_itr->path().string());
                img = imread(img_name, CV_LOAD_IMAGE_GRAYSCALE);
                std::vector<cv::Rect> found;
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
        std::vector<cv::Rect> found;
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
		("walk,k", po::value<bool>()->default_value(false), "Stream images to a HOG person detector")
		("image,e", po::value<std::string>(), "Input Image")
		("stream,s", po::value<std::string>(),
		 "Folders with image stream separated by a comma. If loading features/descriptors, pass folders directly")

		("gpu,u", po::value<bool>()->default_value(false), "Use GPU? Only for specific algorithms") 

		("verbose,v", po::value<int>()->default_value(0), "Debug levels: 0: no info, 1: pipeline stages, 2: all") 
		;

	po::variables_map vm;
	po::store(po::parse_command_line(ac, av, desc), vm);
	po::notify(vm);    

	if (vm.count("help")) {
		std::cout << desc << "\n";
		return vm;
	}
	return vm;
}

int main( int argc, char** argv )
{
	po::variables_map vm = parse_opts(argc, argv);
    
	if(vm["walk"].as<bool>()){
		exec_hog(vm);
	}else{
		cout << "For help: " << argv[0] << " --help" << endl;
	}

	return 0;
}
