#ifndef FACE_AS_H
#define FACE_AS_H

#include <string>
#include <vector>

// OpenCV
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

// ONNX Runtime
#include <onnxruntime_cxx_api.h>

struct SpoofResult {
  float score;
  bool spoof;
  SpoofResult(float score, bool spoof) : score(score), spoof(spoof) {}
};

class FaceAntiSpoofing {
 public:
  FaceAntiSpoofing(const std::string& ckpt, const cv::Size& imgsz = {128, 128},
                   const bool& cuda = false, const float threshold = 0.8);

  void load_model(const std::string& ckpt);
  SpoofResult inference(cv::Mat& image);
  std::vector<float> preprocess(cv::Mat& image);

 private:
  std::string ckpt;
  bool cuda;
  float threshold;
  cv::Size imgsz;

  Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "FaceAntiSpoofing"};
  std::unique_ptr<Ort::Session> session;
  Ort::AllocatorWithDefaultOptions allocator;
  std::vector<std::string> input_names_str;
  std::vector<std::string> output_names_str;
  std::vector<const char*> input_names_cstr;
  std::vector<const char*> output_names_cstr;
};

#endif  // FACE_AS_H
