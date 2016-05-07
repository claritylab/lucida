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

class StoredImage;
class QueryImage;

class Image {
private:
	std::unique_ptr<cv::Mat> desc;
	static std::string saveToFS(const std::string &data);
	static std::vector<float> matToVector(std::unique_ptr<cv::Mat> mat);
	static const int OPENCV_TYPE = CV_32F;
public:
	Image(std::unique_ptr<cv::Mat> _desc) { desc = std::move(_desc); }
	static std::unique_ptr<cv::Mat> imageToMatObj(const std::string &data);
	static const std::string imageToMatString(const std::string &data);
	static std::unique_ptr<cv::Mat> matStringToMatObj(const std::string &mat);
	static int match(
			std::vector<std::unique_ptr<StoredImage>> &train_images,
			std::unique_ptr<QueryImage> query_image);
	static bool matEqual(std::unique_ptr<cv::Mat> a,
			std::unique_ptr<cv::Mat> b);
};

class StoredImage : public Image {
private:
	const std::string label;
public:
	StoredImage(const std::string &_label, std::unique_ptr<cv::Mat> _desc) :
				Image(std::move(_desc)), label(_label) {}
	const std::string getLabel() { return label; }
};

class QueryImage : public Image {
private:
public:
	QueryImage(std::unique_ptr<cv::Mat> _desc) :
		Image(std::move(_desc)) {}
};

#endif
