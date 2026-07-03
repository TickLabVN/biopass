#include "ir_liveness_analyzer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace {

using biopass::ir_liveness::acceptedFramesBracketDarkPulse;
using biopass::ir_liveness::calculateFrameStats;
using biopass::ir_liveness::isDarkPulseFrame;
using biopass::ir_liveness::isIlluminatedFrame;
using biopass::ir_liveness::validateIlluminationPulse;

ImageRGB makeImage(int width, int height, const std::function<uint8_t(int, int)>& value_at) {
  ImageRGB image(width, height);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const uint8_t value = value_at(x, y);
      image.at(y, x, 0) = value;
      image.at(y, x, 1) = value;
      image.at(y, x, 2) = value;
    }
  }
  return image;
}

ImageRGB makeIlluminatedFaceLikeImage() {
  return makeImage(80, 60, [](int x, int y) {
    int value = 70 + (x * 120 / 79) + (y > 18 && y < 42 ? 30 : 0);
    if ((x - 26) * (x - 26) + (y - 25) * (y - 25) < 18) {
      value = 245;
    }
    if ((x - 54) * (x - 54) + (y - 25) * (y - 25) < 18) {
      value = 245;
    }
    return static_cast<uint8_t>(std::min(value, 255));
  });
}

ImageRGB makeStructuredDarkImage() {
  return makeImage(80, 60, [](int x, int y) {
    const int quadrant = (x >= 40 ? 4 : 0) + (y >= 30 ? 3 : 0);
    return static_cast<uint8_t>(7 + quadrant + ((x + y) % 3));
  });
}

ImageRGB makePureBlackImage() {
  return makeImage(80, 60, [](int, int) { return 0; });
}

ImageRGB makeRandomNoiseBlackImage() {
  return makeImage(
      80, 60, [](int x, int y) { return static_cast<uint8_t>((x * 13 + y * 17 + x * y) % 16); });
}

std::vector<biopass::ir_liveness::FrameStats> statsFor(const std::vector<ImageRGB>& frames) {
  std::vector<biopass::ir_liveness::FrameStats> stats;
  for (const auto& frame : frames) {
    stats.push_back(calculateFrameStats(frame));
  }
  return stats;
}

bool expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    return false;
  }
  return true;
}

}  // namespace

int main() {
  int failures = 0;

  const ImageRGB illuminated = makeIlluminatedFaceLikeImage();
  const ImageRGB structured_dark = makeStructuredDarkImage();
  const ImageRGB pure_black = makePureBlackImage();
  const ImageRGB noise_black = makeRandomNoiseBlackImage();

  const auto illuminated_stats = calculateFrameStats(illuminated);
  const auto structured_dark_stats = calculateFrameStats(structured_dark);
  const auto pure_black_stats = calculateFrameStats(pure_black);
  const auto noise_black_stats = calculateFrameStats(noise_black);

  failures += !expect(isIlluminatedFrame(illuminated_stats),
                      "face-like bright IR image is classified as illuminated");
  failures += !expect(isDarkPulseFrame(structured_dark_stats),
                      "structured almost-black IR image is classified as a dark pulse");
  failures +=
      !expect(!isDarkPulseFrame(pure_black_stats), "pure black frame is rejected as a dark pulse");
  failures += !expect(!isDarkPulseFrame(noise_black_stats),
                      "random black noise frame is rejected as a dark pulse");

  const auto valid_pulse =
      validateIlluminationPulse(statsFor({illuminated, structured_dark, illuminated}));
  failures += !expect(valid_pulse.passed, "bright-dark-bright sequence validates IR pulse");
  failures += !expect(valid_pulse.transition_count == 2, "valid pulse has two transitions");
  failures += !expect(
      acceptedFramesBracketDarkPulse(statsFor({illuminated, structured_dark, illuminated}), {0, 2}),
      "accepted face frames bracket the dark pulse");
  failures +=
      !expect(!acceptedFramesBracketDarkPulse(
                  statsFor({illuminated, illuminated, structured_dark, illuminated}), {0, 1}),
              "accepted face frames before the dark pulse do not satisfy bracketing");
  failures +=
      !expect(acceptedFramesBracketDarkPulse(
                  statsFor({illuminated, illuminated, structured_dark, illuminated}), {0, 3}),
              "accepted face frames may bracket the dark pulse across extra frames");

  const auto constant_replay =
      validateIlluminationPulse(statsFor({illuminated, illuminated, illuminated}));
  failures += !expect(!constant_replay.passed, "constant captured IR replay is rejected");

  const auto pure_black_replay =
      validateIlluminationPulse(statsFor({illuminated, pure_black, illuminated}));
  failures += !expect(!pure_black_replay.passed, "pure black plus captured IR replay is rejected");

  const auto noise_black_replay =
      validateIlluminationPulse(statsFor({illuminated, noise_black, illuminated}));
  failures += !expect(!noise_black_replay.passed, "random-noise black replay is rejected");

  const auto dark_before_only =
      validateIlluminationPulse(statsFor({structured_dark, illuminated, illuminated}));
  failures +=
      !expect(!dark_before_only.passed, "dark frame must be captured between illuminated frames");

  return failures == 0 ? 0 : 1;
}
