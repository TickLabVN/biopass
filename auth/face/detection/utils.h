#ifndef FACE_DET_UTILS_H
#define FACE_DET_UTILS_H

// OpenCV
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

// Torch
#include <torch/torch.h>
#include <torch/script.h>

using torch::indexing::Slice;
using torch::indexing::None;


float generate_scale(cv::Mat& image, const std::vector<int>& target_size);
float letterbox(cv::Mat &input_image, cv::Mat &output_image, const std::vector<int> &target_size);
torch::Tensor xyxy2xywh(const torch::Tensor& x);
torch::Tensor xywh2xyxy(const torch::Tensor& x);
// Reference: https://github.com/pytorch/vision/blob/main/torchvision/csrc/ops/cpu/nms_kernel.cpp
torch::Tensor nms(const torch::Tensor& bboxes, const torch::Tensor& scores, float iou_threshold);
torch::Tensor non_max_suppression(torch::Tensor& prediction, float conf_thres=0.25, float iou_thres=0.45, int max_det=10);
torch::Tensor clip_boxes(torch::Tensor& boxes, const std::vector<int>& shape);
torch::Tensor scale_boxes(const std::vector<int>& img1_shape, torch::Tensor& boxes, const std::vector<int>& img0_shape);

#endif // FACE_DET_UTILS_H
