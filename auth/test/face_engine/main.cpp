#include <iostream>
#include <vector>
#include <getopt.h>

#include <opencv2/opencv.hpp>

// Torch
#include <torch/script.h> 
#include <torch/torch.h>
#include <memory>

#include "face_detection.h"
#include "face_recognition.h"

using namespace std;
using namespace cv;


cv::Mat preprocess_face(
    const cv::Mat& img, const cv::Size& shape = cv::Size(224, 224), 
    const std::vector<float>& mean = {0.485, 0.456, 0.406}, 
    const std::vector<float>& std = {0.229, 0.224, 0.225}) {
    
    cv::Mat resized_img;
    cv::resize(img, resized_img, shape);

    // Convert BGR (OpenCV default) to RGB
    cv::cvtColor(resized_img, resized_img, cv::COLOR_BGR2RGB);

    // Convert image to float32 and normalize to range [0, 1]
    resized_img.convertTo(resized_img, CV_32F, 1.0 / 255.0);

    // Normalize each channel
    std::vector<cv::Mat> channels(3);
    cv::split(resized_img, channels); // Split into channels
    for (size_t i = 0; i < channels.size(); ++i) {
        channels[i] = (channels[i] - mean[i]) / std[i];
    }
    cv::merge(channels, resized_img); // Merge channels back

    return resized_img;
}

cv::Mat softmax(const cv::Mat& input) {
    // Ensure input is of type CV_64F
    cv::Mat input64f;
    input.convertTo(input64f, CV_64F);

    // Subtract the maximum value from each row for numerical stability
    cv::Mat maxVal;
    cv::reduce(input64f, maxVal, 1, cv::REDUCE_MAX);  // Reduce across rows (axis=1)
    
    // Normalize the input by subtracting the max value
    cv::Mat inputNormalized = input64f - maxVal;

    // Compute the exponential of each element
    cv::Mat expInput;
    cv::exp(inputNormalized, expInput);

    // Sum of exponentials across rows
    cv::Mat sumExp;
    cv::reduce(expInput, sumExp, 1, cv::REDUCE_SUM);

    // Reshape sumExp to make sure it's a column vector with the correct dimensions for broadcasting
    sumExp = sumExp.reshape(1, expInput.rows); // Ensure sumExp is a column vector

    // Divide each element by the sum of its row
    cv::Mat softmaxOutput;
    cv::divide(expInput, sumExp, softmaxOutput);

    return softmaxOutput;
}

int argmax(const cv::Mat& input) {
    double minVal, maxVal;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc(input, &minVal, &maxVal, &minLoc, &maxLoc);
    std::cout << "Argmax: " << maxLoc << "\n";
    return maxLoc.y;
};

int inference_anti_spoof(cv::dnn::Net& net, const cv::Mat& image) {
    cv::Mat preprocess_image = preprocess_face(image);
    cv::Mat blob;
    cv::dnn::blobFromImage(preprocess_image, blob, 1.0, cv::Size(224, 224), cv::Scalar(), true, false);
    net.setInput(blob);

    std::vector<cv::Mat> outputs;
    net.forward(outputs, net.getUnconnectedOutLayersNames());
    cv::Mat result;
    for (size_t i = 0; i < outputs.size(); ++i) {
        if (i == 0) {
            result = outputs[i];
        } else {
            cv::vconcat(result, outputs[i], result);  // Stack vertically
        }
    }

    cv::Mat softmax_result = softmax(result);
    std::cout << "Result: " << result << "\n";
    std::cout << "Softmax result: " << softmax_result << "\n";
    int cls = argmax(softmax_result);
    return cls;
};


int main(int argc, char **argv) {
    // Face det model
    FaceDetection model_face_det("./weights/yolov11n-face.torchscript");

    // Face reg model
    FaceRecognition model_face_reg("./weights/edgeface_s_gamma_05_ts.pt");
    
    // Face1 Inference
    std::vector<Detection> det_images;
    cv::Mat image = cv::imread(argv[1]);
    det_images = model_face_det.inference(image);

    cv::Mat face1 = det_images[0].image;
    std::string file_name = std::string("result1.jpg");
    cv::imwrite(file_name, face1);

    std::cout << "Face1 saved at results0.jpg\n";
    std::cout << "\n\n";

    // Face2 Inference
    image = cv::imread(argv[2]);
    det_images = model_face_det.inference(image);

    cv::Mat face2 = det_images[0].image;
    file_name = std::string("result2.jpg");
    cv::imwrite(file_name, face2);
    
    std::cout << "Face2 saved at results0.jpg\n";
    std::cout << "\n\n";

    MatchResult match_result = model_face_reg.match(face1, face2);
    std::cout << "Face matching result (close to 1 mean look similar, over than 0.5 mean they might be the same person): " << match_result.dist << std::endl;
    if (match_result.similar) std::cout << "Same person!\n";
    else std::cout << "Different person!\n"; 
}