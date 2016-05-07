#ifndef DETECT_H
#define DETECT_H

#include <assert.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <memory>
#include <stdio.h>
#include <tesseract/baseapi.h>
#include <tesseract/strngs.h>
#include <sys/time.h>
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

class Image {
private:
	const std::string data;
	cv::Mat desc;
	void setDesc();

public:
	const std::string label;
	Image(const std::string &_data);
	Image(const std::string &_label, const std::string &_data);
	static std::string saveToFS(const std::string &data);
	static int match(
			std::vector<std::unique_ptr<Image>> &train_images,
			std::unique_ptr<Image> query_image);
};

#endif
