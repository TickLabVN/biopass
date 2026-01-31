#ifndef FACE_AS_H
#define FACE_AS_H

#include <string>

// OpenCV
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

// Torch
#include <torch/torch.h>
#include <torch/script.h>

class SpoofResult {
public:
    float score;
    bool  spoof;
    SpoofResult(float score, bool spoof) { this->score = score; this->spoof = spoof; }
};

class FaceAntiSpoofing {
public:
    FaceAntiSpoofing(
        // TorchScript model path
        const std::string &ckpt,
        const cv::Size &imgsz={128, 128},
        const bool &cuda=false,
        const float threshold=0.8
    );

    void load_model(const std::string& ckpt);
    SpoofResult inference(cv::Mat& image);
    torch::Tensor preprocess(cv::Mat& image);

private:
    std::string ckpt;
    bool  cuda;
    float threshold;
    cv::Size imgsz;
    torch::jit::script::Module model;
    torch::Device device = torch::Device(torch::kCPU);
};


class FaceAntiSpoofing2 {
public:
    FaceAntiSpoofing2(
        const std::string &ckpt1,
        const std::string &ckpt2,
        const cv::Size &imgsz={80, 80},
        const bool &cuda=false,
        const float threshold=0.45
    );

    void load_model(torch::jit::script::Module& model, const std::string& ckpt2);
    SpoofResult inference(cv::Mat& image);
    torch::Tensor preprocess(cv::Mat& image);
    
private:
    std::string ckpt1;
    std::string ckpt2;
    bool  cuda;
    float threshold;
    cv::Size imgsz;
    torch::jit::script::Module model1;
    torch::jit::script::Module model2;
    torch::Device device = torch::Device(torch::kCPU);
};

#endif // FACE_AS_H
