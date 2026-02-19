#include "face_auth.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <opencv2/opencv.hpp>
#include <thread>

#include "auth_config.h"
#include "face_as.h"
#include "face_detection.h"
#include "face_recognition.h"

namespace facepass {

bool FaceAuth::is_available() const {
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

  std::vector<std::string> enrolledFaces = facepass::list_user_faces(username);
  if (enrolledFaces.empty()) {
    std::cerr << "FaceAuth: No face enrolled for user " << username << ", skipping" << std::endl;
    return AuthResult::Unavailable;
  }

  std::string recogModelPath = face_config_.recognition.model;
  std::string detectModelPath = face_config_.detection.model;
  if (!std::ifstream(recogModelPath).good() || !std::ifstream(detectModelPath).good()) {
    std::cerr << "FaceAuth: Model files not found for user " << username << ", skipping"
              << std::endl;
    return AuthResult::Unavailable;
  }
  std::unique_ptr<FaceRecognition> faceReg;
  std::unique_ptr<FaceDetection> faceDetector;
  try {
    faceDetector = std::make_unique<FaceDetection>(detectModelPath);
  } catch (const std::exception &e) {
    std::string msg = e.what();
    size_t first_line = msg.find('\n');
    if (first_line != std::string::npos)
      msg = msg.substr(0, first_line);
    std::cerr << "FaceAuth: Failed to load detection model: " << msg << ", skipping" << std::endl;
    return AuthResult::Unavailable;
  }
  try {
    faceReg = std::make_unique<FaceRecognition>(recogModelPath);
  } catch (const std::exception &e) {
    std::string msg = e.what();
    size_t first_line = msg.find('\n');
    if (first_line != std::string::npos)
      msg = msg.substr(0, first_line);
    std::cerr << "FaceAuth: Failed to load recognition model: " << msg << ", skipping" << std::endl;
    return AuthResult::Unavailable;
  }

  cv::Mat loginFace;
  camera >> loginFace;
  if (loginFace.empty()) {
    std::cerr << "FaceAuth: Could not read frame" << std::endl;
    return AuthResult::Retry;
  }

  std::vector<Detection> detectedImages = faceDetector->inference(loginFace);
  if (detectedImages.empty()) {
    std::cerr << "FaceAuth: No face detected" << std::endl;
    return AuthResult::Retry;
  }

  cv::Mat face = detectedImages[0].image;

  if (config.anti_spoof) {
    std::string asModelPath = face_config_.anti_spoofing.model;
    try {
      FaceAntiSpoofing faceAs(asModelPath);
      SpoofResult spoofCheck = faceAs.inference(face);
      if (spoofCheck.spoof) {
        std::cerr << "FaceAuth: Spoof detected, score: " << spoofCheck.score << std::endl;
        return AuthResult::Retry;
      }
    } catch (const std::exception &e) {
      std::cerr << "FaceAuth: Anti-spoofing model failed: " << e.what() << ", skipping check"
                << std::endl;
    }
  }

  // Match against all enrolled faces — succeed if any match
  for (const auto &facePath : enrolledFaces) {
    cv::Mat preparedFace = cv::imread(facePath);
    if (preparedFace.empty())
      continue;
    MatchResult match = faceReg->match(preparedFace, face);
    if (match.similar) {
      return AuthResult::Success;
    }
  }

  return AuthResult::Retry;
}

}  // namespace facepass
