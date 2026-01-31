#ifndef FACE_REG_H
#define FACE_REG_H

#include <string>

// OpenCV
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

// Torch
#include <torch/torch.h>
#include <torch/script.h>

class Recognition {
public:
    torch::Tensor feature;
    Recognition(torch::Tensor& feature) {this->feature = feature;}
};

class MatchResult {
public:
    float dist;
    bool  similar;
    MatchResult(float dist, bool similar) { this->dist = dist; this->similar = similar; }
};

class FaceRecognition {
public:
    FaceRecognition(
        const std::string &ckpt,
        const cv::Size &imgsz={112, 112},
        const bool &cuda=false,
        const float threshold=0.50
    );

    void load_model(const std::string& ckpt);
    torch::Tensor inference(cv::Mat& image);
    torch::Tensor preprocess(cv::Mat& image);

    float cosine(const torch::Tensor& tensor1, const torch::Tensor& tensor2);
    MatchResult match(cv::Mat& image1, cv::Mat& image2);

private:
    std::string ckpt;
    bool  cuda;
    float threshold;
    cv::Size imgsz;
    torch::jit::script::Module model;
    torch::Device device = torch::Device(torch::kCPU);
};


#endif // FACE_REG_H
