#include <getopt.h>

#include <iostream>
#include <memory>
#include <vector>

#include <opencv2/opencv.hpp>

#include "face_detection.h"
#include "face_recognition.h"

using namespace std;
using namespace cv;

int main(int argc, char **argv) {
  // Face det model
  FaceDetection model_face_det("./weights/yolov8n-face.onnx");

  // Face reg model
  FaceRecognition model_face_reg("./weights/edgeface_s_gamma_05.onnx");

  // Face1 Inference
  std::vector<Detection> det_images;
  cv::Mat image = cv::imread(argv[1]);
  det_images = model_face_det.inference(image);

  cv::Mat face1 = det_images[0].image;
  std::string file_name = std::string("result1.jpg");
  cv::imwrite(file_name, face1);

  std::cout << "Face1 saved at result1.jpg\n";
  std::cout << "\n\n";

  // Face2 Inference
  image = cv::imread(argv[2]);
  det_images = model_face_det.inference(image);

  cv::Mat face2 = det_images[0].image;
  file_name = std::string("result2.jpg");
  cv::imwrite(file_name, face2);

  std::cout << "Face2 saved at result2.jpg\n";
  std::cout << "\n\n";

  MatchResult match_result = model_face_reg.match(face1, face2);
  std::cout << "Face matching result (close to 1 mean look similar, over than 0.5 mean they might "
               "be the same person): "
            << match_result.dist << std::endl;
  if (match_result.similar)
    std::cout << "Same person!\n";
  else
    std::cout << "Different person!\n";
}
