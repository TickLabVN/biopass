#pragma once

#include <string>

#include "auth_config.h"
#include "auth_method.h"
#include "image_utils.h"

namespace biopass {

class ICameraCaptureSession;
class FaceDetection;

// shared_detector: the caller's already-loaded face detector, reused for the
// IR presence check instead of loading a second copy of the model.
bool checkAntiSpoof(const FaceMethodConfig& face_config, const std::string& username,
                    const ImageRGB& face, const AuthConfig& config, FaceDetection* shared_detector,
                    ICameraCaptureSession* ir_camera_session = nullptr);

}  // namespace biopass
