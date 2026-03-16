#ifndef FACE_REG_H
#define FACE_REG_H

#include <string>
#include <vector>

// OpenCV
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

// ONNX Runtime
#include <onnxruntime_cxx_api.h>

struct Recognition {
  std::vector<float> feature;
  Recognition(std::vector<float>& feature) : feature(feature) {}
};

struct MatchResult {
  float dist;
  bool similar;
  MatchResult(float dist, bool similar) : dist(dist), similar(similar) {}
};

class FaceRecognition {
 public:
  FaceRecognition(const std::string& ckpt, const cv::Size& imgsz = {112, 112},
                  const bool& cuda = false, const float threshold = 0.50);

  void load_model(const std::string& ckpt);
  std::vector<float> inference(cv::Mat& image);
  std::vector<float> preprocess(cv::Mat& image);

  float cosine(const std::vector<float>& feat1, const std::vector<float>& feat2);
  MatchResult match(cv::Mat& image1, cv::Mat& image2);

 private:
  std::string ckpt;
  bool cuda;
  float threshold;
  cv::Size imgsz;

  Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "FaceRecognition"};
  std::unique_ptr<Ort::Session> session;
  Ort::AllocatorWithDefaultOptions allocator;
  std::vector<std::string> input_names_str;
  std::vector<std::string> output_names_str;
  std::vector<const char*> input_names_cstr;
  std::vector<const char*> output_names_cstr;
};

#endif  // FACE_REG_H
