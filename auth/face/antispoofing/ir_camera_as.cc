#include "ir_camera_as.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <thread>

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
                               ICameraCaptureSession* session, int warmup_delay_ms) {
  spdlog::debug("FaceAuth: IR presence check | device='{}' detection_threshold={:.3f} warmup_delay_ms={}",
                device_path, detection_threshold, warmup_delay_ms);

  if (device_path.empty()) {
    spdlog::error("FaceAuth: IR presence check skipped — device path is empty");
    return false;
  }

  if (!std::ifstream(detection_model_path).good()) {
    spdlog::error("FaceAuth: IR presence check — detection model not found: {}", detection_model_path);
    return false;
  }

  // Optional extra delay after warmup frames to let IR LEDs and auto-exposure stabilise.
  if (warmup_delay_ms > 0) {
    spdlog::debug("FaceAuth: IR presence check — sleeping {}ms for camera stabilisation", warmup_delay_ms);
    std::this_thread::sleep_for(std::chrono::milliseconds(warmup_delay_ms));
  }

  ImageRGB frame;
  if (session && session->isOpen()) {
    spdlog::debug("FaceAuth: IR presence check — capturing from existing open session");
    frame = session->capture();
  } else {
    spdlog::debug("FaceAuth: IR presence check — opening new session on '{}'", device_path);
    frame = captureImageByIRCamera(device_path, kIrCaptureWarmupFrames, kIrCaptureTimeoutMs,
                                   kIrCapturePollIntervalMs);
  }

  if (frame.empty()) {
    spdlog::error("FaceAuth: IR presence check — frame capture failed from '{}'", device_path);
    return false;
  }

  spdlog::debug("FaceAuth: IR presence check — frame captured ({}x{})", frame.width, frame.height);

  try {
    FaceDetection detector(detection_model_path, 640, {"face"}, detection_threshold);
    std::vector<Detection> detections = detector.inference(frame);

    // TODO: This is a face presence check only, NOT a real liveness detector.
    // The YOLO model only checks for any face-shaped bounding box in the IR frame.
    // An attacker holding a printed photo or displaying a photo on a screen will
    // pass this check once the IR camera capture succeeds. A real anti-spoofing
    // solution requires a specialized IR liveness model (e.g. texture analysis,
    // depth verification, or dedicated IR liveness classification).
    // Tracked in upcoming issue.
    if (detections.empty()) {
      spdlog::error(
          "FaceAuth: IR presence check FAILED — no face bounding box detected "
          "(threshold={:.3f}, device='{}')",
          detection_threshold, device_path);
      if (debug) {
        saveFailedFace(username, frame, "ir_no_face");
      }
      return false;
    }

    // Log every detection so the caller can see confidence vs threshold.
    for (size_t i = 0; i < detections.size(); ++i) {
      spdlog::debug("FaceAuth: IR presence check — detection[{}] conf={:.4f} (threshold={:.3f})",
                    i, detections[i].conf, detection_threshold);
    }

    spdlog::debug(
        "FaceAuth: IR presence check PASSED — {} face(s) detected, best conf={:.4f} "
        "(NOTE: presence check only, not liveness)",
        detections.size(), detections[0].conf);
    return true;
  } catch (const std::exception& e) {
    spdlog::error("FaceAuth: IR presence check — exception during detection: {}", e.what());
    return false;
  }
}

}  // namespace biopass
