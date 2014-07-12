#ifndef GPU_FUNCS_H
#define GPU_FUNCS_H

std::vector<KeyPoint> exec_feature_gpu(const Mat& img, const std::string detector_str);

Mat exec_descriptor_gpu(const Mat& img_name, const std::string extractor_str, std::vector<KeyPoint> keypoints);

#endif
