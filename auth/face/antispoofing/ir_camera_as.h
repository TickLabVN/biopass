#pragma once

#include <string>

namespace biopass {

class ICameraCaptureSession;

bool checkAntispoofByIRCamera(const std::string& ir_camera_path, const std::string& detection_model_path,
                              float detection_threshold, const std::string& username,
                              bool debug, ICameraCaptureSession* session = nullptr);

}  // namespace biopass
