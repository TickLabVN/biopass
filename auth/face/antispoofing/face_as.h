#ifndef FACE_AS_H
#define FACE_AS_H

#include <string>
#include <vector>

// Image utilities (replaces OpenCV)
#include "image_utils.h"

// ONNX Runtime
#include <onnxruntime_cxx_api.h>

struct SpoofResult {
  float score;
  bool spoof;
  SpoofResult(float score, bool spoof) : score(score), spoof(spoof) {}
};

class FaceAntiSpoofing {
 public:
  FaceAntiSpoofing(const std::string& ckpt, int imgsz = 128, const float threshold = 0.8);

  void loadModel(const std::string& ckpt);
  SpoofResult inference(const ImageRGB& image);
  std::vector<float> preprocess(const ImageRGB& image);

 private:
  std::string ckpt;
  float threshold;
  int imgsz;

  Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "FaceAntiSpoofing"};
  std::unique_ptr<Ort::Session> session;
  Ort::AllocatorWithDefaultOptions allocator;
  std::vector<std::string> input_names_str;
  std::vector<std::string> output_names_str;
  std::vector<const char*> input_names_cstr;
  std::vector<const char*> output_names_cstr;
};

#endif  // FACE_AS_H
