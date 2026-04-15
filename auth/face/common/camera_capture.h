#pragma once

#include <memory>
#include <optional>
#include <string>

#include "image_utils.h"

namespace biopass {

enum class CameraCaptureFormat {
  Default,
  V4L2Grey,
};

class ICameraCaptureSession {
 public:
  virtual ~ICameraCaptureSession() = default;
  virtual bool isOpen() const = 0;
  virtual ImageRGB capture() = 0;
};

bool checkCameraAvailability(const std::optional<std::string>& device_path);
std::unique_ptr<ICameraCaptureSession> openCameraSession(
    const std::optional<std::string>& device_path,
    CameraCaptureFormat format = CameraCaptureFormat::Default,
    int warmup_frames = 5, int capture_timeout_ms = 10000, int poll_interval_ms = 10);
ImageRGB captureImage(
    const std::optional<std::string>& device_path,
    CameraCaptureFormat format = CameraCaptureFormat::Default);
ImageRGB captureImageByIRCamera(const std::string& device_path, int warmup_frames = 5,
                              int capture_timeout_ms = 3000, int poll_interval_ms = 10);

}  // namespace biopass
