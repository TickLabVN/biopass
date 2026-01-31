#ifndef FACE_DET_H
#define FACE_DET_H

// CPP native
#include <fstream>
#include <vector>
#include <string>
#include <random>

// OpenCV
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

// Libtorch
#include <torch/torch.h>
#include <torch/script.h>

class Box {
public:
    int x1, y1, x2, y2;
    Box(int _x1=0, int _y1=0, int _x2=0, int _y2=0) { x1 = _x1; x2 = _x2; y1 = _y1; y2 = _y2; }
};

class Detection {
public:
    int class_id{-1};
    cv::Rect box;
    Box xyxy_box;
    cv::Mat image;
    float conf{0.0};
    cv::Scalar color;
    std::string class_name;

    Detection(int class_id, std::string class_name, float conf, cv::Rect box, Box xyxy_box, const cv::Mat& image, cv::Scalar color) {
        this->class_id = class_id;
        this->class_name = class_name;
        this->conf = conf;
        this->box = box;
        this->xyxy_box = xyxy_box;
        this->color = color;
        this->image = image;
    };

    bool operator>(const Detection& obj) const {
        return (this->box.width * this->box.height) > (obj.box.width * obj.box.height);
    };

    bool operator<(const Detection& obj) const {
        return (this->box.width * this->box.height) < (obj.box.width * obj.box.height);
    };
};

class FaceDetection {
public:
    FaceDetection(
        const std::string &ckpt,
        const cv::Size &imgsz={640, 640},
        const std::vector<std::string> &classes={"face"},
        const bool &cuda=false,
        const float conf=0.50,
        const float iou=0.50
    );
    
    void load_model(const std::string& ckpt);
    std::vector<Detection> inference(cv::Mat& image);
    torch::Tensor preprocess(cv::Mat& image);
    
private:
    bool  cuda;
    float conf;
    float iou;
    cv::Size imgsz;
    std::string ckpt{};
    torch::jit::script::Module model;
    std::vector<std::string> classes{"face"};
    torch::Device device = torch::Device(torch::kCPU);
    
};

#endif // FACE_DET_H