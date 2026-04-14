#include "face_detection.h"

#include <algorithm>

#include "utils.h"

FaceDetection::FaceDetection(const std::string &ckpt, int imgsz,
                             const std::vector<std::string> &classes, const float conf,
                             const float iou) {
  this->ckpt = ckpt;
  this->imgsz = imgsz;
  this->conf = conf;
  this->iou = iou;
  this->classes = classes;

  this->loadModel(ckpt);
}

void FaceDetection::loadModel(const std::string &ckpt) {
  this->ckpt = ckpt;

  Ort::SessionOptions opts;
  opts.SetIntraOpNumThreads(1);
  opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

  this->session = std::make_unique<Ort::Session>(this->env, ckpt.c_str(), opts);

  this->input_names_str.clear();
  this->output_names_str.clear();
  this->input_names_cstr.clear();
  this->output_names_cstr.clear();

  for (size_t i = 0; i < this->session->GetInputCount(); i++) {
    auto name = this->session->GetInputNameAllocated(i, this->allocator);
    this->input_names_str.push_back(name.get());
  }
  for (size_t i = 0; i < this->session->GetOutputCount(); i++) {
    auto name = this->session->GetOutputNameAllocated(i, this->allocator);
    this->output_names_str.push_back(name.get());
  }
  for (auto &s : this->input_names_str)
    this->input_names_cstr.push_back(s.c_str());
  for (auto &s : this->output_names_str)
    this->output_names_cstr.push_back(s.c_str());
}

std::vector<Detection> FaceDetection::inference(const ImageRGB &image) {
  // Preprocess
  ImageRGB input_image = imageLetterbox(image, this->imgsz, this->imgsz);
  std::vector<float> image_data = this->preprocess(input_image);

  // Inference
  std::vector<int64_t> input_shape = {1, 3, (int64_t)this->imgsz, (int64_t)this->imgsz};
  auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
  Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
      memory_info, image_data.data(), image_data.size(), input_shape.data(), input_shape.size());

  auto output_tensors =
      this->session->Run(Ort::RunOptions{nullptr}, this->input_names_cstr.data(), &input_tensor, 1,
                         this->output_names_cstr.data(), this->output_names_cstr.size());

  auto &out = output_tensors[0];
  auto shape = out.GetTensorTypeAndShapeInfo().GetShape();
  int pred_dim = static_cast<int>(shape[1]);
  int num_preds = static_cast<int>(shape[2]);
  const float *output_data = out.GetTensorData<float>();

  // NMS
  auto raw_dets = non_max_suppression(output_data, num_preds, pred_dim, this->conf, this->iou);
  scale_boxes({input_image.height, input_image.width}, raw_dets, {image.height, image.width});

  // Get detection results
  std::vector<Detection> results;
  for (auto &d : raw_dets) {
    int x1 = std::max(0, (int)d.x1);
    int y1 = std::max(0, (int)d.y1);
    int x2 = std::min(image.width, (int)d.x2);
    int y2 = std::min(image.height, (int)d.y2);

    // Ensure the box has positive area after clipping
    if (x2 - x1 <= 0 || y2 - y1 <= 0) {
      continue;
    }

    Box xyxy_box(x1, y1, x2, y2);
    ImageRGB crop_face = image.crop(x1, y1, x2, y2);
    Detection det(d.cls, std::string("face"), d.conf, xyxy_box, crop_face);
    results.push_back(det);
  }

  std::sort(results.begin(), results.end(), std::greater<Detection>());
  return results;
}

std::vector<float> FaceDetection::preprocess(const ImageRGB &input_image) {
  return imageToChw(input_image);
}
