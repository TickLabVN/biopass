#pragma once

#include <string>

namespace biopass {

bool checkAntispoofByIRCamera(const std::string& ir_camera_path, const std::string& detection_model_path,
                              float detection_threshold, const std::string& username,
                              bool debug);

}  // namespace biopass
