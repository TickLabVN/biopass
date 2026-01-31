#include "face_as.h"

int torch_argmax(const torch::Tensor& input) {
    torch::Tensor flat_tensor = input.to(torch::kCPU).contiguous().view(-1);
    int max_index = flat_tensor.argmax().item<int>();
    return max_index;
};

torch::Tensor face_as_to_tensor(const cv::Mat& img) {
	torch::Tensor img_tensor = torch::from_blob(img.data, {img.rows, img.cols, 3}, torch::kUInt8);
	img_tensor = img_tensor / 255.0;
	img_tensor = img_tensor.permute({2, 0, 1}).to(torch::kFloat32);
	return img_tensor;
};

torch::Tensor face_as_normalize(const torch::Tensor& input, const std::vector<float>& mean, const std::vector<float>& std) {
    TORCH_CHECK(input.size(0) == mean.size(), "Mean size must match number of channels");
    TORCH_CHECK(input.size(0) == std.size(), "Std size must match number of channels");

    torch::Tensor normalize = input.clone();
    for (size_t i = 0; i < mean.size(); ++i) {
        normalize[i] = (normalize[i] - mean[i]) / std[i];
    }
    return normalize.unsqueeze(0);
}

FaceAntiSpoofing::FaceAntiSpoofing(
    const std::string &ckpt,
    const cv::Size &imgsz,
    const bool &cuda,
    const float threshold)
{
    this->ckpt  = ckpt;
    this->imgsz = imgsz;
    this->cuda  = cuda;
    this->threshold = threshold;
    if (this->cuda) this->device = torch::Device(torch::kCUDA);
    else this->device = torch::Device(torch::kCPU);
    this->load_model(ckpt);
}

void FaceAntiSpoofing::load_model(const std::string& ckpt)
{
    this->ckpt = ckpt;
    this->model = torch::jit::load(this->ckpt);
    this->model.eval();
    this->model.to(this->device, torch::kFloat32);
}

torch::Tensor FaceAntiSpoofing::preprocess(cv::Mat& input_image) {
    std::vector<float> mean = {0.5931, 0.4690, 0.4229};
    std::vector<float> std = {0.2471, 0.2214, 0.2157};
    int height = 128, width = 128;
    cv::Mat resize_img, rgb_img, float_img;
    cv::resize(input_image, resize_img, cv::Size(width, height));
    cv::cvtColor(resize_img, rgb_img, cv::COLOR_BGR2RGB);

    torch::Tensor img_tensor = face_as_to_tensor(rgb_img);
    torch::Tensor norm_img_tensor = face_as_normalize(img_tensor, mean, std);

    return norm_img_tensor;
}

SpoofResult FaceAntiSpoofing::inference(cv::Mat& image) {
    torch::Tensor input_image = this->preprocess(image);
    torch::jit::IValue input(input_image);

	std::vector<torch::jit::IValue> inputs;
	inputs.push_back(input);

	torch::Tensor output = this->model.forward(inputs).toTensor().squeeze();

    int spoof_cls = torch_argmax(output);
    float score = output.view(-1)[spoof_cls].item<float>();
    SpoofResult result(score, spoof_cls == 1 && score >= this->threshold);
    return result;
}