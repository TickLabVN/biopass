#include "face_auth.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <thread>

#include "auth_config.h"
#include "face_as.h"
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

  std::string userFacePath = facepass::user_face_path(username);
  // If face not enrolled for this user, skip transparently
  if (!std::ifstream(userFacePath).good()) {
    std::cerr << "FaceAuth: No face enrolled for user " << username << ", skipping" << std::endl;
    return AuthResult::Unavailable;
  }
  cv::Mat preparedFace = cv::imread(userFacePath);
  if (preparedFace.empty()) {
    std::cerr << "FaceAuth: Face not registered for user " << username << std::endl;
    return AuthResult::Unavailable;
  }

  std::string recogModelPath = facepass::model_path(username, facepass::FACE_RECOGNITION);
  std::string detectModelPath = facepass::model_path(username, facepass::FACE_DETECTION);
  // If models are missing, skip transparently (they live in user data)
  if (!std::ifstream(recogModelPath).good() || !std::ifstream(detectModelPath).good()) {
    std::cerr << "FaceAuth: Model files not found for user " << username << ", skipping"
              << std::endl;
    return AuthResult::Unavailable;
  }
  FaceRecognition faceReg(recogModelPath);
  FaceDetection faceDetector(detectModelPath);

  cv::Mat loginFace;
  camera >> loginFace;
  if (loginFace.empty()) {
    std::cerr << "FaceAuth: Could not read frame" << std::endl;
    return AuthResult::Retry;
  }

  std::vector<Detection> detectedImages = faceDetector.inference(loginFace);
  if (detectedImages.empty()) {
    std::cerr << "FaceAuth: No face detected" << std::endl;
    return AuthResult::Retry;
  }

  cv::Mat face = detectedImages[0].image;

  if (config.anti_spoof) {
    FaceAntiSpoofing faceAs(facepass::model_path(username, facepass::FACE_ANTI_SPOOFING));
    SpoofResult spoofCheck = faceAs.inference(face);
    if (spoofCheck.spoof) {
      std::cerr << "FaceAuth: Spoof detected, score: " << spoofCheck.score << std::endl;
      return AuthResult::Retry;
    }
  }

  MatchResult match = faceReg.match(preparedFace, face);
  if (match.similar) {
    return AuthResult::Success;
  }

  return AuthResult::Failure;
}

}  // namespace facepass
