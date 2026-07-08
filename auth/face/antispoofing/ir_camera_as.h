#pragma once

#include <string>

namespace biopass {

class ICameraCaptureSession;
class FaceDetection;

// IR face-presence check. Reuses the caller's already-loaded face detector
// (the same one used for the main visual-camera detection) rather than
// loading a second copy of the model.
//
// Returns true when the detector finds at least one bounding box in the
// captured IR frame.
//
// IMPORTANT — this is NOT a liveness detector. It verifies that a face shape
// is visible in the IR stream; a printed photo placed in front of an IR camera
// can still pass this check. Treat it as a "blank-frame guard" rather than
// anti-spoofing in the strict sense.
//
// warmup_delay_ms: extra sleep (ms) inserted once, before the first capture,
// giving IR LEDs and auto-exposure time to stabilise.
//
// presence_timeout_ms: total wall-clock budget for retrying capture+inference
// until a face is found. The IR emitter blinks, so a single frame may be
// unusable (all-white or all-dark); this lets the check retry rather than
// fail on the first bad frame. 0 disables retry (single attempt).
bool checkAntispoofByIRCamera(const std::string& ir_camera_path, FaceDetection* detector,
                              const std::string& username, bool debug,
                              ICameraCaptureSession* session = nullptr, int warmup_delay_ms = 300,
                              int presence_timeout_ms = 1500);

}  // namespace biopass
