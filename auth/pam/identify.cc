#include "identify.h"

#include <memory>

namespace uuid {
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<> dis(0, 15);
static std::uniform_int_distribution<> dis2(8, 11);

string v4() {
  stringstream ss;
  ss << std::hex;

  for (int i = 0; i < 8; i++) ss << dis(gen);

  ss << "-";
  for (int i = 0; i < 4; i++) ss << dis(gen);

  ss << "-4";
  for (int i = 0; i < 3; i++) ss << dis(gen);

  ss << "-";
  ss << dis2(gen);
  for (int i = 0; i < 3; i++) ss << dis(gen);

  ss << "-";
  for (int i = 0; i < 12; i++) ss << dis(gen);

  return ss.str();
}
}  // namespace uuid

namespace {
bool save_failed_face(const string &username, const cv::Mat &face, const string &reason) {
  string failedFacePath = facepass::debug_path(username) + "/" + reason + "." + uuid::v4() + ".jpg";
  if (!cv::imwrite(failedFacePath, face)) {
    cerr << "ERROR: Could not save failed face to " << failedFacePath << endl;
    return false;
  }
  return true;
}

bool process_anti_spoofing(FaceAntiSpoofing &faceAs, cv::Mat &face) {
  SpoofResult spoofCheck = faceAs.inference(face);
  if (spoofCheck.spoof) {
    cerr << "ERROR: Spoof detected, score: " << spoofCheck.score << endl;
    return false;
  }
  return true;
}
void sleep_for(int ms) { this_thread::sleep_for(chrono::milliseconds(ms)); }
}  // namespace

int scan_face(const string &username, const facepass::FaceMethodConfig &face_config, int8_t retries,
              const int gap, bool anti_spoofing) {
  cv::VideoCapture camera(0, cv::CAP_V4L2);
  if (!camera.isOpened()) {
    cerr << "ERROR: Could not open camera" << endl;
    return PAM_AUTH_ERR;
  }

  std::vector<std::string> enrolledFaces = facepass::list_user_faces(username);
  if (enrolledFaces.empty()) {
    cerr << "ERROR: No face enrolled for user " << username << endl;
    return PAM_AUTH_ERR;
  }

  std::unique_ptr<FaceRecognition> faceReg;
  std::unique_ptr<FaceDetection> faceDetector;
  std::unique_ptr<FaceAntiSpoofing> faceAs;
  try {
    faceDetector = std::make_unique<FaceDetection>(face_config.detection.model);
  } catch (const std::exception &e) {
    std::string msg = e.what();
    size_t first_line = msg.find('\n');
    if (first_line != std::string::npos)
      msg = msg.substr(0, first_line);
    cerr << "ERROR: Failed to load detection model: " << msg << endl;
    return PAM_AUTH_ERR;
  }
  try {
    faceReg = std::make_unique<FaceRecognition>(face_config.recognition.model);
  } catch (const std::exception &e) {
    std::string msg = e.what();
    size_t first_line = msg.find('\n');
    if (first_line != std::string::npos)
      msg = msg.substr(0, first_line);
    cerr << "ERROR: Failed to load recognition model: " << msg << endl;
    return PAM_AUTH_ERR;
  }
  if (anti_spoofing) {
    try {
      faceAs = std::make_unique<FaceAntiSpoofing>(face_config.anti_spoofing.model);
    } catch (const std::exception &e) {
      std::string msg = e.what();
      size_t first_line = msg.find('\n');
      if (first_line != std::string::npos)
        msg = msg.substr(0, first_line);
      cerr << "ERROR: Failed to load anti-spoofing model: " << msg << endl;
      // Continue without anti-spoofing if it fails?
      // identify.cc seems to depend on it if anti_spoofing is true.
      return PAM_AUTH_ERR;
    }
  }

  bool success = false;
  while (retries--) {
    cv::Mat loginFace;
    camera >> loginFace;
    if (loginFace.empty()) {
      cerr << "ERROR: Could not read frame" << endl;
      break;
    }

    std::vector<Detection> detectedImages = faceDetector->inference(loginFace);
    if (detectedImages.empty()) {
      cerr << "ERROR: No face detected" << endl;
      sleep_for(gap);
      continue;
    }

    cv::Mat face = detectedImages[0].image;
    if (anti_spoofing && !process_anti_spoofing(*faceAs, face)) {
      save_failed_face(username, face, "spoof");
      sleep_for(gap);
      continue;
    }

    // Match against all enrolled faces — succeed if any match
    bool matched = false;
    for (const auto &facePath : enrolledFaces) {
      cv::Mat preparedFace = cv::imread(facePath);
      if (preparedFace.empty())
        continue;
      MatchResult match = faceReg->match(preparedFace, face);
      if (match.similar) {
        matched = true;
        break;
      }
    }

    if (matched) {
      success = true;
      break;
    }

    save_failed_face(username, face, "not similar");
    sleep_for(gap);
  }

  return success ? PAM_SUCCESS : PAM_AUTH_ERR;
}
