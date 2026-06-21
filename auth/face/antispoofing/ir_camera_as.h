#pragma once

#include <string>
#include <vector>

#include "image_utils.h"

class FaceDetection;
class FaceAntiSpoofing;

namespace biopass {

class ICameraCaptureSession;

struct IRLivenessResult {
  bool face_presence_confirmed = false;
  bool model_passed = false;
};

// IR face liveness classification.
// Captures frames from the IR camera, retries if they are dark, detects faces,
// filters by size, and scores them using the anti-spoofing model.
IRLivenessResult checkAntispoofByIRCamera(ICameraCaptureSession* ir_camera_session,
                                          const std::string& ir_camera_path,
                                          FaceDetection* ir_det_model,
                                          FaceAntiSpoofing* ir_as_model, int warmup_delay_ms,
                                          float min_face_area_ratio, const std::string& username,
                                          bool debug_enabled);

// IR face liveness classification for faces that have already been detected and cropped.
bool checkAntispoofByIRCrops(const std::vector<ImageRGB>& ir_faces, FaceAntiSpoofing* face_as,
                             const std::string& username, bool debug);

}  // namespace biopass
