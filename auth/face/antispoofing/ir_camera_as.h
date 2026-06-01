#pragma once

#include <string>

namespace biopass {

class ICameraCaptureSession;

// IR face-presence check.
// Returns true when the YOLO face-detection model finds at least one bounding
// box in the captured IR frame.
//
// IMPORTANT — this is NOT a liveness detector. It verifies that a face shape
// is visible in the IR stream; a printed photo placed in front of an IR camera
// can still pass this check. Treat it as a "blank-frame guard" rather than
// anti-spoofing in the strict sense.
//
// warmup_delay_ms: extra sleep (ms) inserted after the V4L2 warmup frames and
// before the capture, giving IR LEDs and auto-exposure time to stabilise.
bool checkAntispoofByIRCamera(const std::string& ir_camera_path,
                               const std::string& detection_model_path,
                               float detection_threshold, const std::string& username,
                               bool debug, ICameraCaptureSession* session = nullptr,
                               int warmup_delay_ms = 300);

}  // namespace biopass
