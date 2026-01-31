#include "face_detection.h"
#include "utils.h"
#include <algorithm>

FaceDetection::FaceDetection(
    const std::string &ckpt, 
    const cv::Size &imgsz, 
    const std::vector<std::string> &classes, 
    const bool &cuda,
    const float conf,
    const float iou)
{
    this->ckpt  = ckpt;
    this->imgsz = imgsz;
    this->cuda  = cuda;
    this->conf  = conf;
    this->iou   = iou;   
    this->classes = classes;
    if (this->cuda) this->device = torch::Device(torch::kCUDA);
    else this->device = torch::Device(torch::kCPU);

    this->load_model(ckpt);
}

void FaceDetection::load_model(const std::string& ckpt)
{
    this->ckpt = ckpt;
    this->model = torch::jit::load(this->ckpt);
    this->model.eval();
    this->model.to(this->device, torch::kFloat32);
}

std::vector<Detection> FaceDetection::inference(cv::Mat& image) {
    // Preprocess
    cv::Mat input_image;
    letterbox(image, input_image, {640, 640});
    cv::cvtColor(input_image, input_image, cv::COLOR_BGR2RGB);
    torch::Tensor image_tensor = this->preprocess(input_image);
    std::vector<torch::jit::IValue> inputs {image_tensor};
    
    // Inference
    torch::Tensor output = this->model.forward(inputs).toTensor().cpu();

    // NMS
    auto keep = non_max_suppression(output)[0];
    auto boxes = keep.index({Slice(), Slice(None, 4)});
    keep.index_put_({Slice(), Slice(None, 4)}, scale_boxes({input_image.rows, input_image.cols}, boxes, {image.rows, image.cols}));

    // Get detection results
    std::vector<Detection> results;
    for (int i = 0; i < keep.size(0); i++) {
        int x1 = keep[i][0].item().toFloat();
        int y1 = keep[i][1].item().toFloat();
        int x2 = keep[i][2].item().toFloat();
        int y2 = keep[i][3].item().toFloat();
        float conf = keep[i][4].item().toFloat();
        int cls = keep[i][5].item().toInt();
        
        cv::Rect box(x1, y1, x2 - x1, y2 - y1);
        cv::Scalar color(0, 255, 0);
        Box xyxy_box(x1, y1, x2, y2);
        cv::Mat crop_face = image(box);
        Detection det(cls, std::string("face"), conf, box, xyxy_box, crop_face, color);
        results.push_back(det);
    }

    std::sort(results.begin(), results.end(), std::greater<Detection>());
    return results;
}

torch::Tensor FaceDetection::preprocess(cv::Mat& input_image) {
    torch::Tensor image_tensor = torch::from_blob(input_image.data, {input_image.rows, input_image.cols, 3}, torch::kByte).to(device);
    image_tensor = image_tensor.toType(torch::kFloat32).div(255);
    image_tensor = image_tensor.permute({2, 0, 1});
    image_tensor = image_tensor.unsqueeze(0);
    return image_tensor;
}