#include "face_as.h"

FaceAntiSpoofing::FaceAntiSpoofing(const std::string& ckpt, int imgsz, const float threshold) {
  this->ckpt = ckpt;
  this->imgsz = imgsz;
  this->threshold = threshold;
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
  for (auto& s : this->input_names_str) this->input_names_cstr.push_back(s.c_str());
  for (auto& s : this->output_names_str) this->output_names_cstr.push_back(s.c_str());
}

std::vector<float> FaceAntiSpoofing::preprocess(const ImageRGB& input_image) {
  const float mean[3] = {0.5931f, 0.4690f, 0.4229f};
  const float std[3] = {0.2471f, 0.2214f, 0.2157f};
  ImageRGB resize_img = resizeImage(input_image, this->imgsz, this->imgsz);

  return imageToChwNormalized(resize_img, mean, std);
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

  if (output_tensors.empty()) {
    throw std::runtime_error("FaceAntiSpoofing: model returned no outputs");
  }

  const auto shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();
  if (shape.empty() || shape.back() != 2) {
    throw std::runtime_error("FaceAntiSpoofing: unexpected output shape, expected [..., 2]");
  }

  // The model outputs post-softmax probabilities in [0, 1].
  // Index 0 = REAL, index 1 = SPOOF.
  const float* logits = output_tensors[0].GetTensorData<float>();
  const float real_score = logits[0];
  const float spoof_score = logits[1];

  // Secure threshold logic: only allow passage if the real score is high-confidence
  // (>= threshold) AND strictly greater than the spoof score.
  // Otherwise, fail-closed (treat as spoof).
  const bool is_real = (real_score >= this->threshold) && (real_score > spoof_score);

  return SpoofResult(real_score, spoof_score, is_real);
}
