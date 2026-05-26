#include "face_detection.h"

#include <algorithm>
#include <cmath>

#include "utils.h"

static float sigmoid(float x) {
  return 1.0f / (1.0f + std::exp(-x));
}

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

  // Decode all YOLOv8 output heads
  const int strides[3] = {8, 16, 32};
  std::vector<RawDet> candidates;
  for (size_t ti = 0; ti < output_tensors.size() && ti < 3; ++ti) {
    auto &t = output_tensors[ti];
    auto ts = t.GetTensorTypeAndShapeInfo().GetShape();
    if (ts.size() < 3) continue;
    int pred_dim = static_cast<int>(ts[1]);
    int H = static_cast<int>(ts[2]);
    int W = static_cast<int>(ts.size() > 3 ? ts[3] : 1);
    int num = H * W;
    const float *tdata = t.GetTensorData<float>();
    int stride = strides[ti];

    if (pred_dim < 65 || H == 0 || W == 0) continue;

    // ONNX tensor is NCHW layout: tdata[ch * H * W + h * W + w]
    // We access by iterating spatial positions (h, w)
    for (int row = 0; row < H; ++row) {
      for (int col = 0; col < W; ++col) {
        int sp = row * W + col;  // spatial offset within a channel

        // YOLOv8-pose: class score at channel 64
        float score = sigmoid(tdata[64 * num + sp]);
        if (score < this->conf) continue;

        // DFL decode: softmax over 16 bins per coordinate
        auto dfl = [&](int ch_base) -> float {
          float mx = tdata[ch_base * num + sp];
          for (int j = 1; j < 16; ++j) {
            float v = tdata[(ch_base + j) * num + sp];
            if (v > mx) mx = v;
          }
          float sum = 0.0f, result = 0.0f;
          for (int j = 0; j < 16; ++j) {
            float e = std::exp(tdata[(ch_base + j) * num + sp] - mx);
            sum += e;
            result += j * e;
          }
          return result / sum;
        };

        float lt = dfl(0);   // left distance from cell center
        float tp = dfl(16);  // top distance from cell center
        float rb = dfl(32);  // right distance from cell center
        float bt = dfl(48);  // bottom distance from cell center

        // Cell center at (col+0.5, row+0.5) in grid units
        float cx_cell = (col + 0.5f) * stride;
        float cy_cell = (row + 0.5f) * stride;

        RawDet d;
        d.x1 = cx_cell - lt * stride;
        d.y1 = cy_cell - tp * stride;
        d.x2 = cx_cell + rb * stride;
        d.y2 = cy_cell + bt * stride;
        d.conf = score;
        d.cls = 0;
        candidates.push_back(d);
      }
    }
  }

  // NMS on decoded boxes
  auto keep_indices = nms_boxes(candidates, iou);
  scale_boxes({input_image.height, input_image.width}, candidates, {image.height, image.width});

  // Get detection results
  std::vector<Detection> results;
  for (int idx : keep_indices) {
    auto &d = candidates[idx];
    int x1 = std::max(0, (int)d.x1);
    int y1 = std::max(0, (int)d.y1);
    int x2 = std::min(image.width, (int)d.x2);
    int y2 = std::min(image.height, (int)d.y2);

    if (x2 - x1 <= 0 || y2 - y1 <= 0) continue;

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
