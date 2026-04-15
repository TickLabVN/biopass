#include "ir_camera_as.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <fstream>

#include "camera_capture.h"
#include "debug_image_io.h"
#include "face_detection.h"

namespace biopass {

namespace {

constexpr int kIrCaptureWarmupFrames = 5;
constexpr int kIrCaptureTimeoutMs = 3000;
constexpr int kIrCapturePollIntervalMs = 10;

}  // namespace

bool checkAntispoofByIRCamera(const std::string& device_path,
                              const std::string& detection_model_path, float detection_threshold,
                              const std::string& username, bool debug,
                              ICameraCaptureSession* session) {
  if (device_path.empty()) {
    return false;
  }

  if (!std::ifstream(detection_model_path).good()) {
    spdlog::error("FaceAuth: Detection model file not found: {}", detection_model_path);
    return false;
  }

  ImageRGB frame;
  if (session && session->isOpen()) {
    frame = session->capture();
  } else {
    frame = captureImageByIRCamera(device_path, kIrCaptureWarmupFrames, kIrCaptureTimeoutMs,
                                   kIrCapturePollIntervalMs);
  }
  if (frame.empty()) {
    spdlog::error("FaceAuth: IR camera failed to capture frame from {}", device_path);
    return false;
  }

  try {
    FaceDetection detector(detection_model_path, 640, {"face"}, detection_threshold);
    if (detector.inference(frame).empty()) {
      spdlog::error("FaceAuth: IR camera check failed, no face detected in captured frame");
      if (debug) {
        saveFailedFace(username, frame, "ir_no_face");
      }
      return false;
    }

    return true;
  } catch (const std::exception& e) {
    spdlog::error("FaceAuth: IR anti-spoofing check failed: {}", e.what());
    return false;
  }
}
}  // namespace biopass
