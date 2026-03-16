#ifndef FACE_DET_UTILS_H
#define FACE_DET_UTILS_H

// CPP native
#include <vector>

// OpenCV
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

struct RawDet {
  float x1, y1, x2, y2, conf;
  int cls;
};

float generate_scale(cv::Mat& image, const std::vector<int>& target_size);
float letterbox(cv::Mat& input_image, cv::Mat& output_image, const std::vector<int>& target_size);

std::vector<RawDet> non_max_suppression(const float* output, int num_preds, int pred_dim,
                                        float conf_thres = 0.25, float iou_thres = 0.45,
                                        int max_det = 300);

void scale_boxes(const std::vector<int>& img1_shape, std::vector<RawDet>& dets,
                 const std::vector<int>& img0_shape);

#endif  // FACE_DET_UTILS_H
