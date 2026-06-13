#pragma once

#include <string>

#include "auth_config.h"
#include "auth_method.h"
#include "image_utils.h"

struct Detection;
class FaceAntiSpoofing;
class FaceDetection;

namespace biopass {

class ICameraCaptureSession;

bool checkAntiSpoof(const FaceMethodConfig& face_config, const std::string& username,
                    const Detection& rgb_det, const AuthConfig& config,
                    ICameraCaptureSession* ir_camera_session = nullptr,
                    FaceAntiSpoofing* rgb_as_model = nullptr, FaceDetection* ir_det_model = nullptr,
                    FaceAntiSpoofing* ir_as_model = nullptr);

}  // namespace biopass
