#include "face_auth.h"

#include <chrono>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <thread>

#include "face_as.h"
#include "face_config.h"
#include "face_detection.h"
#include "face_recognition.h"

namespace facepass {

bool FaceAuth::is_available() const {
  // Check if a camera is available
  cv::VideoCapture camera(0, cv::CAP_V4L2);
  bool available = camera.isOpened();
  camera.release();
  return available;
}

AuthResult FaceAuth::authenticate(const std::string &username, const AuthConfig &config) {
  cv::VideoCapture camera(0, cv::CAP_V4L2);
  if (!camera.isOpened()) {
    std::cerr << "FaceAuth: Could not open camera" << std::endl;
    return AuthResult::Unavailable;
  }

  std::string userFacePath = user_face_path(username);
  cv::Mat preparedFace = cv::imread(userFacePath);
  if (preparedFace.empty()) {
    std::cerr << "FaceAuth: Face not registered for user " << username << std::endl;
    return AuthResult::Failure;
  }

  FaceRecognition faceReg(model_path(username, FACE_RECOGNITION));
  FaceDetection faceDetector(model_path(username, FACE_DETECTION));
  std::unique_ptr<FaceAntiSpoofing> faceAs = nullptr;

  if (config.anti_spoof) {
    faceAs = std::make_unique<FaceAntiSpoofing>(model_path(username, FACE_ANTI_SPOOFING));
  }

  int retries = config.retries;
  while (retries-- > 0) {
    cv::Mat loginFace;
    camera >> loginFace;
    if (loginFace.empty()) {
      std::cerr << "FaceAuth: Could not read frame" << std::endl;
      break;
    }

    std::vector<Detection> detectedImages = faceDetector.inference(loginFace);
    if (detectedImages.empty()) {
      std::cerr << "FaceAuth: No face detected" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(config.retry_delay_ms));
      continue;
    }

    cv::Mat face = detectedImages[0].image;

    if (config.anti_spoof && faceAs) {
      SpoofResult spoofCheck = faceAs->inference(face);
      if (spoofCheck.spoof) {
        std::cerr << "FaceAuth: Spoof detected, score: " << spoofCheck.score << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(config.retry_delay_ms));
        continue;
      }
    }

    MatchResult match = faceReg.match(preparedFace, face);
    if (match.similar) {
      return AuthResult::Success;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(config.retry_delay_ms));
  }

  return AuthResult::Failure;
}

}  // namespace facepass
