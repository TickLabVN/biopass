#include "face_detection.h"

#include <algorithm>

#include "utils.h"

FaceDetection::FaceDetection(const std::string &ckpt, const cv::Size &imgsz,
                             const std::vector<std::string> &classes, const bool &cuda,
                             const float conf, const float iou) {
  this->ckpt = ckpt;
  this->imgsz = imgsz;
  this->cuda = cuda;
  this->conf = conf;
  this->iou = iou;
  this->classes = classes;

  this->load_model(ckpt);
}

void FaceDetection::load_model(const std::string &ckpt) {
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

std::vector<Detection> FaceDetection::inference(cv::Mat &image) {
  // Preprocess
  cv::Mat input_image;
  letterbox(image, input_image, {640, 640});
  cv::cvtColor(input_image, input_image, cv::COLOR_BGR2RGB);
  std::vector<float> image_data = this->preprocess(input_image);

  // Inference
  std::vector<int64_t> input_shape = {1, 3, this->imgsz.height, this->imgsz.width};
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
  scale_boxes({input_image.rows, input_image.cols}, raw_dets, {image.rows, image.cols});

  // Get detection results
  std::vector<Detection> results;
  for (auto &d : raw_dets) {
    int x1 = d.x1;
    int y1 = d.y1;
    int x2 = d.x2;
    int y2 = d.y2;
    float det_conf = d.conf;
    int cls = d.cls;

    // Clip bounding box to image boundaries to prevent OpenCV assertion failures
    x1 = std::max(0, x1);
    y1 = std::max(0, y1);
    x2 = std::min(image.cols, x2);
    y2 = std::min(image.rows, y2);

    cv::Rect box(x1, y1, x2 - x1, y2 - y1);
    cv::Scalar color(0, 255, 0);
    Box xyxy_box(x1, y1, x2, y2);

    // Ensure the box has positive area after clipping
    if (box.width <= 0 || box.height <= 0) {
      continue;
    }

    cv::Mat crop_face = image(box);
    Detection det(cls, std::string("face"), det_conf, box, xyxy_box, crop_face, color);
    results.push_back(det);
  }

  std::sort(results.begin(), results.end(), std::greater<Detection>());
  return results;
}

std::vector<float> FaceDetection::preprocess(cv::Mat &input_image) {
  int h = input_image.rows;
  int w = input_image.cols;
  std::vector<float> data(3 * h * w);

  // HWC -> CHW, normalize to [0,1]
  for (int c = 0; c < 3; c++) {
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        data[c * h * w + y * w + x] = input_image.at<cv::Vec3b>(y, x)[c] / 255.0f;
      }
    }
  }
  return data;
}
