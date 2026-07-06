#include "face_as.h"

#include <algorithm>
#include <cmath>

int argmax(const float* data, int size) {
  int max_index = 0;
  float max_val = data[0];
  for (int i = 1; i < size; i++) {
    if (data[i] > max_val) {
      max_val = data[i];
      max_index = i;
    }
  }
  return max_index;
}

FaceAntiSpoofing::FaceAntiSpoofing(const std::string& ckpt, int imgsz, const float threshold,
                                   const std::string& model_type) {
  this->ckpt = ckpt;
  this->imgsz = imgsz;
  this->threshold = threshold;
  this->model_type = model_type;

  // Unrecognized model_type: infer from the checkpoint filename.
  if (this->model_type != "minifasv2" && this->model_type != "mobilenetv3") {
    this->model_type = (ckpt.find("mobilenetv3") != std::string::npos) ? "mobilenetv3" : "minifasv2";
  }

  this->loadModel(ckpt);
}

void FaceAntiSpoofing::loadModel(const std::string& ckpt) {
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

std::vector<float> FaceAntiSpoofing::preprocessMobileNetV3(const ImageRGB& image) {
  const float mean[3] = {0.5931f, 0.4690f, 0.4229f};
  const float std[3] = {0.2471f, 0.2214f, 0.2157f};
  ImageRGB resize_img = resizeImage(image, this->imgsz, this->imgsz);

  return imageToChwNormalized(resize_img, mean, std);
}

std::vector<float> FaceAntiSpoofing::preprocessMiniFASv2(const ImageRGB& image) {
  ImageRGB letterboxed = imageLetterboxReflect101(image, this->imgsz);
  return imageToChw(letterboxed);
}

std::vector<float> FaceAntiSpoofing::preprocess(const ImageRGB& image) {
  if (this->model_type == "mobilenetv3") {
    return this->preprocessMobileNetV3(image);
  }
  return this->preprocessMiniFASv2(image);
}

SpoofResult FaceAntiSpoofing::inference(const ImageRGB& image) {
  std::vector<float> input_data = this->preprocess(image);

  std::vector<int64_t> input_shape = {1, 3, (int64_t)this->imgsz, (int64_t)this->imgsz};
  auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
  Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
      memory_info, input_data.data(), input_data.size(), input_shape.data(), input_shape.size());

  auto output_tensors =
      this->session->Run(Ort::RunOptions{nullptr}, this->input_names_cstr.data(), &input_tensor, 1,
                         this->output_names_cstr.data(), this->output_names_cstr.size());

  const float* logits = output_tensors[0].GetTensorData<float>();

  if (this->model_type == "mobilenetv3") {
    // Index 0: Spoof, Index 1: Real
    int spoof_cls = argmax(logits, 2);
    float score = logits[spoof_cls];
    return SpoofResult(score, spoof_cls == 0 && score >= this->threshold);
  }

  // minifasv2: logits[0] = real_logit, logits[1] = spoof_logit
  float max_logit = std::max(logits[0], logits[1]);
  float exp_real = std::exp(logits[0] - max_logit);
  float exp_spoof = std::exp(logits[1] - max_logit);
  float spoof_prob = exp_spoof / (exp_real + exp_spoof);
  return SpoofResult(spoof_prob, spoof_prob >= this->threshold);
}
