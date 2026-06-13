#pragma once

#include <string>
#include <vector>

#include "image_utils.h"

class FaceDetection;
class FaceAntiSpoofing;

namespace biopass {

class ICameraCaptureSession;

// IR face liveness classification.
// Captures frames from the IR camera, retries if they are dark, detects faces,
// filters by size, and scores them using the anti-spoofing model.
bool checkAntispoofByIRCamera(ICameraCaptureSession* ir_camera_session,
                              const std::string& ir_camera_path, FaceDetection* ir_det_model,
                              FaceAntiSpoofing* ir_as_model, int warmup_delay_ms,
                              float min_face_area_ratio, const std::string& username,
                              bool debug_enabled, bool ai_enabled, bool ir_model_hard_fail);

// IR face liveness classification for faces that have already been detected and cropped.
bool checkAntispoofByIRCrops(const std::vector<ImageRGB>& ir_faces, FaceAntiSpoofing* face_as,
                             const std::string& username, bool debug, bool is_diagnostic = false);

}  // namespace biopass
