#ifndef FACE_REG_H
#define FACE_REG_H

#include <string>
#include <vector>

// Image utilities (replaces OpenCV)
#include "image_utils.h"

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
  FaceRecognition(const std::string& ckpt, int imgsz = 112, const float threshold = 0.50);

  void loadModel(const std::string& ckpt);
  std::vector<float> inference(const ImageRGB& image);
  std::vector<float> preprocess(const ImageRGB& image);

  float cosine(const std::vector<float>& feat1, const std::vector<float>& feat2);
  MatchResult match(const ImageRGB& image1, const ImageRGB& image2);

 private:
  std::string ckpt;
  float threshold;
  int imgsz;

  Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "FaceRecognition"};
  std::unique_ptr<Ort::Session> session;
  Ort::AllocatorWithDefaultOptions allocator;
  std::vector<std::string> input_names_str;
  std::vector<std::string> output_names_str;
  std::vector<const char*> input_names_cstr;
  std::vector<const char*> output_names_cstr;
};

#endif  // FACE_REG_H
