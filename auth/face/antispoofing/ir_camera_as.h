#pragma once

#include <string>

#include "auth_config.h"

namespace biopass {

class ICameraCaptureSession;

using IRCaptureParams = IRCaptureConfig;

bool checkAntispoofByIRCamera(const std::string& ir_camera_path, const std::string& detection_model_path,
                              float detection_threshold, const std::string& username,
                              bool debug, ICameraCaptureSession* session = nullptr,
                              const IRCaptureParams& ir_params = IRCaptureParams{});

}  // namespace biopass
