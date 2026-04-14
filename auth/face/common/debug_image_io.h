#pragma once

#include <string>

#include "image_utils.h"

namespace biopass {

void saveFailedFace(const std::string& username, const ImageRGB& face, const std::string& reason);

}  // namespace biopass
