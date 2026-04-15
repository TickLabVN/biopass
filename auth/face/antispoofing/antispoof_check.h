#pragma once

#include <string>

#include "auth_config.h"
#include "auth_method.h"
#include "image_utils.h"

namespace biopass {

class ICameraCaptureSession;

bool checkAntiSpoof(const FaceMethodConfig& face_config, const std::string& username,
                    const ImageRGB& face, const AuthConfig& config,
                    ICameraCaptureSession* ir_camera_session = nullptr);

}  // namespace biopass
