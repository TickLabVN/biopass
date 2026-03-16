#include "face_as.h"

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

FaceAntiSpoofing::FaceAntiSpoofing(const std::string& ckpt, const cv::Size& imgsz,
                                   const bool& cuda, const float threshold) {
  this->ckpt = ckpt;
  this->imgsz = imgsz;
  this->cuda = cuda;
  this->threshold = threshold;
  this->load_model(ckpt);
}

void FaceAntiSpoofing::load_model(const std::string& ckpt) {
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

std::vector<float> FaceAntiSpoofing::preprocess(cv::Mat& input_image) {
  std::vector<float> mean = {0.5931, 0.4690, 0.4229};
  std::vector<float> std = {0.2471, 0.2214, 0.2157};
  int height = 128, width = 128;
  cv::Mat resize_img, rgb_img;
  cv::resize(input_image, resize_img, cv::Size(width, height));
  cv::cvtColor(resize_img, rgb_img, cv::COLOR_BGR2RGB);

  std::vector<float> data(3 * height * width);
  for (int c = 0; c < 3; c++) {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        float val = rgb_img.at<cv::Vec3b>(y, x)[c] / 255.0f;
        data[c * height * width + y * width + x] = (val - mean[c]) / std[c];
      }
    }
  }
  return data;
}

SpoofResult FaceAntiSpoofing::inference(cv::Mat& image) {
  std::vector<float> input_data = this->preprocess(image);

  std::vector<int64_t> input_shape = {1, 3, this->imgsz.height, this->imgsz.width};
  auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
  Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
      memory_info, input_data.data(), input_data.size(), input_shape.data(), input_shape.size());

  auto output_tensors =
      this->session->Run(Ort::RunOptions{nullptr}, this->input_names_cstr.data(), &input_tensor, 1,
                         this->output_names_cstr.data(), this->output_names_cstr.size());

  const float* logits = output_tensors[0].GetTensorData<float>();

  int spoof_cls = argmax(logits, 2);
  float score = logits[spoof_cls];
  return SpoofResult(score, spoof_cls == 1 && score >= this->threshold);
}