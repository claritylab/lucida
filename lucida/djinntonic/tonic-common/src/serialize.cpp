#include <iostream>
#include <string>
#include <sstream>
#include <stdio.h>

#include "opencv2/opencv.hpp"
#include "serialize.h"

using namespace cv;
using namespace std;

// Serialize a cv::Mat to a string
string serialize(Mat input, string filename){
    // We will need to also serialize the width, height, type and size of the matrix
    int width = input.cols;
    int height = input.rows;
    int type = input.type();
    size_t size = input.total() * input.elemSize();

    // Initialize a stringstream and write the data
    stringstream ss;
    ss.write((char*)(&width), sizeof(int));
    ss.write((char*)(&height), sizeof(int));
    ss.write((char*)(&type), sizeof(int));
    ss.write((char*)(&size), sizeof(size_t));

    // Write the whole image data
    ss.write((char*)input.data, size);
    ss << filename;

    return ss.str();
}

Mat deserialize(stringstream& input, string &filename){
    // The data we need to deserialize
    int width = 0;
    int height = 0;
    int type = 0;
    size_t size = 0;

    // Read the width, height, type and size of the buffer
    input.read((char*)(&width), sizeof(int));
    input.read((char*)(&height), sizeof(int));
    input.read((char*)(&type), sizeof(int));
    input.read((char*)(&size), sizeof(size_t));

    // Allocate a buffer for the pixels
    char* data = new char[size];
    // Read the pixels from the stringstream
    input.read(data, size);

    //set filename
    input >> filename;

    // Construct the image (clone it so that it won't need our buffer anymore)
    Mat m = Mat(height, width, type, data).clone();

    // Delete our buffer
    delete[]data;

    // Return the matrix
    return m;
}

