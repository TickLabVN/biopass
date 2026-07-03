#include "ir_liveness_analyzer.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace biopass::ir_liveness {

namespace {

constexpr int kTileColumns = 8;
constexpr int kTileRows = 6;

constexpr double kIlluminatedMeanMin = 35.0;
constexpr int kIlluminatedMaxMin = 120;
constexpr double kIlluminatedStddevMin = 6.0;

constexpr double kDarkMeanMax = 18.0;
constexpr int kDarkMaxMax = 80;
constexpr double kDarkStddevMin = 0.75;
constexpr double kDarkStddevMax = 22.0;
constexpr double kDarkTileMeanStddevMin = 0.35;
constexpr double kDarkTileStructureRatioMin = 0.5;

constexpr double kPulseMeanSwingMin = 22.0;

double safeStddev(double sum, double sum_sq, int count) {
  if (count <= 0) {
    return 0.0;
  }

  const double mean = sum / static_cast<double>(count);
  const double variance = std::max(0.0, sum_sq / static_cast<double>(count) - mean * mean);
  return std::sqrt(variance);
}

enum class FrameKind {
  Neutral,
  Illuminated,
  Dark,
};

FrameKind classifyFrame(const FrameStats& stats) {
  if (isIlluminatedFrame(stats)) {
    return FrameKind::Illuminated;
  }
  if (isDarkPulseFrame(stats)) {
    return FrameKind::Dark;
  }
  return FrameKind::Neutral;
}

}  // namespace

FrameStats calculateFrameStats(const ImageRGB& frame) {
  FrameStats stats;
  const int total_pixels = frame.width * frame.height;
  if (frame.empty() || total_pixels <= 0) {
    return stats;
  }

  stats.empty = false;
  stats.min_val = 255;

  double sum_val = 0.0;
  double sum_sq_val = 0.0;
  const uint8_t* ptr = frame.ptr();
  for (int i = 0; i < total_pixels; ++i) {
    const uint8_t val = ptr[i * 3];
    stats.min_val = std::min(stats.min_val, static_cast<int>(val));
    stats.max_val = std::max(stats.max_val, static_cast<int>(val));
    sum_val += val;
    sum_sq_val += static_cast<double>(val) * static_cast<double>(val);
  }

  stats.mean_val = sum_val / static_cast<double>(total_pixels);
  stats.stddev_val = safeStddev(sum_val, sum_sq_val, total_pixels);

  const int tile_cols = std::min(kTileColumns, frame.width);
  const int tile_rows = std::min(kTileRows, frame.height);
  if (tile_cols <= 0 || tile_rows <= 0) {
    return stats;
  }

  std::vector<double> tile_means;
  tile_means.reserve(static_cast<size_t>(tile_cols * tile_rows));
  double tile_stddev_sum = 0.0;

  for (int ty = 0; ty < tile_rows; ++ty) {
    const int y0 = ty * frame.height / tile_rows;
    const int y1 = (ty + 1) * frame.height / tile_rows;
    for (int tx = 0; tx < tile_cols; ++tx) {
      const int x0 = tx * frame.width / tile_cols;
      const int x1 = (tx + 1) * frame.width / tile_cols;

      double tile_sum = 0.0;
      double tile_sum_sq = 0.0;
      int tile_pixels = 0;
      for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
          const uint8_t val = frame.at(y, x, 0);
          tile_sum += val;
          tile_sum_sq += static_cast<double>(val) * static_cast<double>(val);
          ++tile_pixels;
        }
      }

      if (tile_pixels <= 0) {
        continue;
      }

      tile_means.push_back(tile_sum / static_cast<double>(tile_pixels));
      tile_stddev_sum += safeStddev(tile_sum, tile_sum_sq, tile_pixels);
    }
  }

  if (tile_means.empty()) {
    return stats;
  }

  double tile_mean_sum = 0.0;
  double tile_mean_sum_sq = 0.0;
  for (double mean : tile_means) {
    tile_mean_sum += mean;
    tile_mean_sum_sq += mean * mean;
  }

  stats.tile_mean_stddev =
      safeStddev(tile_mean_sum, tile_mean_sum_sq, static_cast<int>(tile_means.size()));
  stats.tile_stddev_mean = tile_stddev_sum / static_cast<double>(tile_means.size());
  return stats;
}

bool isIlluminatedFrame(const FrameStats& stats) {
  return !stats.empty && stats.mean_val >= kIlluminatedMeanMin &&
         stats.max_val >= kIlluminatedMaxMin && stats.stddev_val >= kIlluminatedStddevMin;
}

bool isDarkPulseFrame(const FrameStats& stats) {
  return !stats.empty && stats.mean_val <= kDarkMeanMax && stats.max_val <= kDarkMaxMax &&
         stats.stddev_val >= kDarkStddevMin && stats.stddev_val <= kDarkStddevMax &&
         stats.tile_mean_stddev >= kDarkTileMeanStddevMin &&
         stats.tile_mean_stddev >= stats.tile_stddev_mean * kDarkTileStructureRatioMin;
}

PulseValidationResult validateIlluminationPulse(const std::vector<FrameStats>& frames) {
  PulseValidationResult result;
  if (frames.size() < 3) {
    result.reason = "too few IR frames to validate an illumination pulse";
    return result;
  }

  std::vector<int> illuminated_indices;
  std::vector<int> dark_indices;
  std::vector<FrameKind> compressed_pattern;

  double brightest_illuminated_mean = 0.0;
  double darkest_pulse_mean = std::numeric_limits<double>::max();
  for (size_t i = 0; i < frames.size(); ++i) {
    const FrameKind kind = classifyFrame(frames[i]);
    if (kind == FrameKind::Illuminated) {
      illuminated_indices.push_back(static_cast<int>(i));
      brightest_illuminated_mean = std::max(brightest_illuminated_mean, frames[i].mean_val);
    } else if (kind == FrameKind::Dark) {
      dark_indices.push_back(static_cast<int>(i));
      darkest_pulse_mean = std::min(darkest_pulse_mean, frames[i].mean_val);
    }

    if (kind != FrameKind::Neutral &&
        (compressed_pattern.empty() || compressed_pattern.back() != kind)) {
      compressed_pattern.push_back(kind);
    }
  }

  result.illuminated_frame_count = static_cast<int>(illuminated_indices.size());
  result.dark_frame_count = static_cast<int>(dark_indices.size());
  result.transition_count =
      compressed_pattern.empty() ? 0 : static_cast<int>(compressed_pattern.size()) - 1;

  if (illuminated_indices.size() < 2) {
    result.reason = "fewer than two illuminated IR frames were observed";
    return result;
  }
  if (dark_indices.empty()) {
    result.reason = "no valid dark IR pulse frame was observed";
    return result;
  }

  if (darkest_pulse_mean < std::numeric_limits<double>::max()) {
    result.mean_swing = brightest_illuminated_mean - darkest_pulse_mean;
  }
  if (result.mean_swing < kPulseMeanSwingMin) {
    result.reason = "IR brightness did not vary enough to match pulsed illumination";
    return result;
  }

  for (int dark_index : dark_indices) {
    const auto before = std::find_if(illuminated_indices.begin(), illuminated_indices.end(),
                                     [dark_index](int index) { return index < dark_index; });
    const auto after = std::find_if(illuminated_indices.begin(), illuminated_indices.end(),
                                    [dark_index](int index) { return index > dark_index; });
    if (before != illuminated_indices.end() && after != illuminated_indices.end()) {
      result.pulse_span_frames = *after - *before;
      result.passed = result.transition_count >= 2;
      if (!result.passed) {
        result.reason = "IR brightness pattern did not include a bright-dark-bright transition";
      }
      return result;
    }
  }

  result.reason = "dark IR pulse frame was not captured between illuminated IR frames";
  return result;
}

bool acceptedFramesBracketDarkPulse(const std::vector<FrameStats>& frames,
                                    const std::vector<int>& accepted_frame_indices) {
  if (accepted_frame_indices.size() < 2) {
    return false;
  }

  for (size_t i = 0; i < accepted_frame_indices.size(); ++i) {
    for (size_t j = i + 1; j < accepted_frame_indices.size(); ++j) {
      const int first = accepted_frame_indices[i];
      const int second = accepted_frame_indices[j];
      if (first < 0 || second >= static_cast<int>(frames.size()) || first >= second) {
        continue;
      }

      for (int frame_index = first + 1; frame_index < second; ++frame_index) {
        if (isDarkPulseFrame(frames[frame_index])) {
          return true;
        }
      }
    }
  }

  return false;
}

}  // namespace biopass::ir_liveness
