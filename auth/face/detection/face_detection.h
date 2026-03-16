#ifndef FACE_DET_H
#define FACE_DET_H

// CPP native
#include <fstream>
#include <random>
#include <string>
#include <vector>

// OpenCV
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

// ONNX Runtime
#include <onnxruntime_cxx_api.h>

struct Box {
  int x1, y1, x2, y2;
  Box(int x1 = 0, int y1 = 0, int x2 = 0, int y2 = 0) : x1(x1), y1(y1), x2(x2), y2(y2) {}
};

struct Detection {
  int class_id{-1};
  cv::Rect box;
  Box xyxy_box;
  cv::Mat image;
  float conf{0.0};
  cv::Scalar color;
  std::string class_name;

  Detection(int class_id, std::string class_name, float conf, cv::Rect box, Box xyxy_box,
            const cv::Mat& image, cv::Scalar color)
      : class_id(class_id),
        class_name(class_name),
        conf(conf),
        box(box),
        xyxy_box(xyxy_box),
        color(color),
        image(image) {}

  bool operator>(const Detection& obj) const {
    return (box.width * box.height) > (obj.box.width * obj.box.height);
  }

  bool operator<(const Detection& obj) const {
    return (box.width * box.height) < (obj.box.width * obj.box.height);
  }
};

class FaceDetection {
 public:
  FaceDetection(const std::string& ckpt, const cv::Size& imgsz = {640, 640},
                const std::vector<std::string>& classes = {"face"}, const bool& cuda = false,
                const float conf = 0.50, const float iou = 0.50);

  void load_model(const std::string& ckpt);
  std::vector<Detection> inference(cv::Mat& image);
  std::vector<float> preprocess(cv::Mat& image);

 private:
  bool cuda;
  float conf;
  float iou;
  cv::Size imgsz;
  std::string ckpt{};
  std::vector<std::string> classes{"face"};

  Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "FaceDetection"};
  std::unique_ptr<Ort::Session> session;
  Ort::AllocatorWithDefaultOptions allocator;
  std::vector<std::string> input_names_str;
  std::vector<std::string> output_names_str;
  std::vector<const char*> input_names_cstr;
  std::vector<const char*> output_names_cstr;
};

#endif  // FACE_DET_H