#pragma once

#include <string>
#include <vector>

#include "image_utils.h"

namespace biopass::ir_liveness {

struct FrameStats {
  bool empty = true;
  int min_val = 0;
  int max_val = 0;
  double mean_val = 0.0;
  double stddev_val = 0.0;
  double tile_mean_stddev = 0.0;
  double tile_stddev_mean = 0.0;
};

struct PulseValidationResult {
  bool passed = false;
  int illuminated_frame_count = 0;
  int dark_frame_count = 0;
  int transition_count = 0;
  int pulse_span_frames = 0;
  double mean_swing = 0.0;
  std::string reason;
};

FrameStats calculateFrameStats(const ImageRGB& frame);
bool isIlluminatedFrame(const FrameStats& stats);
bool isDarkPulseFrame(const FrameStats& stats);
PulseValidationResult validateIlluminationPulse(const std::vector<FrameStats>& frames);
bool acceptedFramesBracketDarkPulse(const std::vector<FrameStats>& frames,
                                    const std::vector<int>& accepted_frame_indices);

}  // namespace biopass::ir_liveness
