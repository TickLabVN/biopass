#include "face_recognition.h"

#include <cmath>

FaceRecognition::FaceRecognition(const std::string& ckpt, int imgsz, const float threshold) {
  this->ckpt = ckpt;
  this->imgsz = imgsz;
  this->threshold = threshold;
  this->loadModel(ckpt);
}

void FaceRecognition::loadModel(const std::string& ckpt) {
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
  for (auto& s : this->input_names_str)
    this->input_names_cstr.push_back(s.c_str());
  for (auto& s : this->output_names_str)
    this->output_names_cstr.push_back(s.c_str());
}

std::vector<float> FaceRecognition::preprocess(const ImageRGB& input_image) {
  ImageRGB resize_img = imageResizePad(input_image, this->imgsz, this->imgsz);

  const float mean[3] = {0.5f, 0.5f, 0.5f};
  const float std[3] = {0.5f, 0.5f, 0.5f};

  return imageToChwNormalized(resize_img, mean, std);
}

std::vector<float> FaceRecognition::inference(const ImageRGB& image) {
  std::vector<float> input_data = this->preprocess(image);

  std::vector<int64_t> input_shape = {1, 3, (int64_t)this->imgsz, (int64_t)this->imgsz};
  auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
  Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
      memory_info, input_data.data(), input_data.size(), input_shape.data(), input_shape.size());

  auto output_tensors =
      this->session->Run(Ort::RunOptions{nullptr}, this->input_names_cstr.data(), &input_tensor, 1,
                         this->output_names_cstr.data(), this->output_names_cstr.size());

  auto& out = output_tensors[0];
  auto shape = out.GetTensorTypeAndShapeInfo().GetShape();
  int embed_dim = static_cast<int>(shape[1]);
  const float* data = out.GetTensorData<float>();

  return std::vector<float>(data, data + embed_dim);
}

float FaceRecognition::cosine(const std::vector<float>& feat1, const std::vector<float>& feat2) {
  float dot_product = 0, norm1 = 0, norm2 = 0;
  for (size_t i = 0; i < feat1.size(); i++) {
    dot_product += feat1[i] * feat2[i];
    norm1 += feat1[i] * feat1[i];
    norm2 += feat2[i] * feat2[i];
  }
  norm1 = std::sqrt(norm1);
  norm2 = std::sqrt(norm2);

  if (norm1 == 0 || norm2 == 0) {
    throw std::runtime_error("One of the tensors has zero magnitude.");
  }

  float sim = dot_product / (norm1 * norm2);
  return sim;
}

MatchResult FaceRecognition::match(const ImageRGB& image1, const ImageRGB& image2) {
  std::vector<float> feature1 = this->inference(image1);
  std::vector<float> feature2 = this->inference(image2);
  float distance = this->cosine(feature1, feature2);
  bool similar = false;
  if (distance > this->threshold)
    similar = true;
  return MatchResult(distance, similar);
}
