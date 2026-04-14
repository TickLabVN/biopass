#ifndef FACE_DET_H
#define FACE_DET_H

// CPP native
#include <fstream>
#include <random>
#include <string>
#include <vector>

// Image utilities (replaces OpenCV)
#include "image_utils.h"

// ONNX Runtime
#include <onnxruntime_cxx_api.h>

struct Box {
  int x1, y1, x2, y2;
  Box(int x1 = 0, int y1 = 0, int x2 = 0, int y2 = 0) : x1(x1), y1(y1), x2(x2), y2(y2) {}
};

struct Detection {
  int class_id{-1};
  Box box;
  ImageRGB image;
  float conf{0.0};
  std::string class_name;

  Detection(int class_id, std::string class_name, float conf, Box box, const ImageRGB& image)
      : class_id(class_id), class_name(class_name), conf(conf), box(box), image(image) {}

  int area() const { return (box.x2 - box.x1) * (box.y2 - box.y1); }

  bool operator>(const Detection& obj) const { return area() > obj.area(); }

  bool operator<(const Detection& obj) const { return area() < obj.area(); }
};

class FaceDetection {
 public:
  FaceDetection(const std::string& ckpt, int imgsz = 640,
                const std::vector<std::string>& classes = {"face"}, const float conf = 0.50,
                const float iou = 0.50);

  void loadModel(const std::string& ckpt);
  std::vector<Detection> inference(const ImageRGB& image);
  std::vector<float> preprocess(const ImageRGB& image);

 private:
  float conf;
  float iou;
  int imgsz;
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
