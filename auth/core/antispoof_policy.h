#pragma once

#include <string>
#include <string_view>

namespace biopass {

enum class IrAntispoofMode {
  Balanced,
  Strict,
};

constexpr IrAntispoofMode parseIrAntispoofMode(std::string_view mode) {
  return mode == "strict" ? IrAntispoofMode::Strict : IrAntispoofMode::Balanced;
}

inline std::string normalizeIrAntispoofMode(std::string_view mode) {
  return parseIrAntispoofMode(mode) == IrAntispoofMode::Strict ? "strict" : "balanced";
}

constexpr bool evaluateAntiSpoofPolicy(IrAntispoofMode mode, bool ai_enabled, bool ai_passed,
                                       bool ir_enabled, bool ir_presence_confirmed,
                                       bool ir_model_passed) {
  if (ai_enabled && ir_enabled) {
    if (!ir_presence_confirmed) {
      return false;
    }
    return mode == IrAntispoofMode::Strict ? ai_passed && ir_model_passed
                                           : ai_passed || ir_model_passed;
  }
  if (ai_enabled) {
    return ai_passed;
  }
  if (ir_enabled) {
    return ir_presence_confirmed && ir_model_passed;
  }
  return true;
}

}  // namespace biopass
