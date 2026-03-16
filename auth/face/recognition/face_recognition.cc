#include "face_recognition.h"

#include <cmath>

cv::Mat resize(const cv::Mat& image, cv::Size target_size,
               cv::Scalar pad_color = cv::Scalar(0, 0, 0), int interp = cv::INTER_AREA) {
  int target_w = target_size.width;
  int target_h = target_size.height;
  int original_h = image.rows;
  int original_w = image.cols;

  // Calculate scaling factor
  double scale = std::min((double)target_w / original_w, (double)target_h / original_h);
  int new_w = static_cast<int>(original_w * scale);
  int new_h = static_cast<int>(original_h * scale);

  // Resize the image
  cv::Mat resizedImage;
  cv::resize(image, resizedImage, cv::Size(new_w, new_h), 0, 0, interp);

  // Calculate padding
  int delta_w = target_w - new_w;
  int delta_h = target_h - new_h;
  int top = delta_h / 2;
  int bottom = delta_h - top;
  int left = delta_w / 2;
  int right = delta_w - left;

  // Add padding
  cv::Mat padded_img;
  cv::copyMakeBorder(resizedImage, padded_img, top, bottom, left, right, cv::BORDER_CONSTANT,
                     pad_color);

  return padded_img;
}

std::vector<float> to_normalized_chw(const cv::Mat& img, const std::vector<float>& mean,
                                     const std::vector<float>& std) {
  int h = img.rows;
  int w = img.cols;
  std::vector<float> data(3 * h * w);

  for (int c = 0; c < 3; c++) {
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        float val = img.at<cv::Vec3b>(y, x)[c] / 255.0f;
        data[c * h * w + y * w + x] = (val - mean[c]) / std[c];
      }
    }
  }
  return data;
}

FaceRecognition::FaceRecognition(const std::string& ckpt, const cv::Size& imgsz, const bool& cuda,
                                 const float threshold) {
  this->ckpt = ckpt;
  this->imgsz = imgsz;
  this->cuda = cuda;
  this->threshold = threshold;
  this->load_model(ckpt);
}

void FaceRecognition::load_model(const std::string& ckpt) {
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

std::vector<float> FaceRecognition::preprocess(cv::Mat& input_image) {
  cv::Mat resize_img;
  resize_img = resize(input_image, cv::Size(112, 112));

  cv::Mat rgb_img;
  cv::cvtColor(resize_img, rgb_img, cv::COLOR_BGR2RGB);

  std::vector<float> mean({0.5, 0.5, 0.5});
  std::vector<float> std({0.5, 0.5, 0.5});

  return to_normalized_chw(rgb_img, mean, std);
}

std::vector<float> FaceRecognition::inference(cv::Mat& image) {
  std::vector<float> input_data = this->preprocess(image);

  std::vector<int64_t> input_shape = {1, 3, this->imgsz.height, this->imgsz.width};
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

MatchResult FaceRecognition::match(cv::Mat& image1, cv::Mat& image2) {
  std::vector<float> feature1 = this->inference(image1);
  std::vector<float> feature2 = this->inference(image2);
  float distance = this->cosine(feature1, feature2);
  bool similar = false;
  if (distance > this->threshold)
    similar = true;
  return MatchResult(distance, similar);
}
