#include "antispoof_policy.h"

#include <iostream>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << '\n';
    failures++;
  }
}

bool expectedPolicy(biopass::IrAntispoofMode mode, bool ai_enabled, bool ai_passed, bool ir_enabled,
                    bool ir_presence_confirmed, bool ir_model_passed) {
  if (!ai_enabled && !ir_enabled) {
    return true;
  }
  if (ai_enabled && !ir_enabled) {
    return ai_passed;
  }
  if (!ai_enabled && ir_enabled) {
    return ir_presence_confirmed && ir_model_passed;
  }
  if (!ir_presence_confirmed) {
    return false;
  }
  if (mode == biopass::IrAntispoofMode::Strict) {
    return ai_passed && ir_model_passed;
  }
  return ai_passed || ir_model_passed;
}

void testModeParsing() {
  using biopass::IrAntispoofMode;

  expect(biopass::parseIrAntispoofMode("balanced") == IrAntispoofMode::Balanced,
         "balanced parses as Balanced");
  expect(biopass::parseIrAntispoofMode("strict") == IrAntispoofMode::Strict,
         "strict parses as Strict");
  expect(biopass::parseIrAntispoofMode("unknown") == IrAntispoofMode::Balanced,
         "unknown modes fail safely to the documented balanced default");
  expect(biopass::parseIrAntispoofMode("") == IrAntispoofMode::Balanced,
         "empty mode uses balanced");
  expect(biopass::normalizeIrAntispoofMode("strict") == "strict", "strict normalization is stable");
  expect(biopass::normalizeIrAntispoofMode("invalid") == "balanced",
         "invalid normalization produces balanced");
}

void testExhaustivePolicyMatrix() {
  using biopass::IrAntispoofMode;

  for (const auto mode : {IrAntispoofMode::Balanced, IrAntispoofMode::Strict}) {
    for (int ai_enabled = 0; ai_enabled <= 1; ++ai_enabled) {
      for (int ai_passed = 0; ai_passed <= 1; ++ai_passed) {
        for (int ir_enabled = 0; ir_enabled <= 1; ++ir_enabled) {
          for (int ir_presence = 0; ir_presence <= 1; ++ir_presence) {
            for (int ir_model = 0; ir_model <= 1; ++ir_model) {
              const bool actual = biopass::evaluateAntiSpoofPolicy(
                  mode, ai_enabled, ai_passed, ir_enabled, ir_presence, ir_model);
              const bool expected =
                  expectedPolicy(mode, ai_enabled, ai_passed, ir_enabled, ir_presence, ir_model);
              expect(actual == expected,
                     "policy matrix mismatch for mode=" +
                         std::string(mode == IrAntispoofMode::Strict ? "strict" : "balanced") +
                         " ai_enabled=" + std::to_string(ai_enabled) + " ai_passed=" +
                         std::to_string(ai_passed) + " ir_enabled=" + std::to_string(ir_enabled) +
                         " ir_presence=" + std::to_string(ir_presence) +
                         " ir_model=" + std::to_string(ir_model));
            }
          }
        }
      }
    }
  }
}

void testSecurityInvariants() {
  using biopass::IrAntispoofMode;

  for (const auto mode : {IrAntispoofMode::Balanced, IrAntispoofMode::Strict}) {
    for (int ai_passed = 0; ai_passed <= 1; ++ai_passed) {
      for (int ir_model = 0; ir_model <= 1; ++ir_model) {
        expect(!biopass::evaluateAntiSpoofPolicy(mode, true, ai_passed, true, false, ir_model),
               "configured IR can never be bypassed without confirmed face presence");
      }
    }
  }

  expect(biopass::evaluateAntiSpoofPolicy(IrAntispoofMode::Balanced, true, false, true, true, true),
         "balanced accepts a strong IR result when RGB AI transiently fails");
  expect(biopass::evaluateAntiSpoofPolicy(IrAntispoofMode::Balanced, true, true, true, true, false),
         "balanced accepts RGB AI when IR presence is confirmed but IR classification misses");
  expect(!biopass::evaluateAntiSpoofPolicy(IrAntispoofMode::Strict, true, false, true, true, true),
         "strict rejects an RGB AI failure");
  expect(!biopass::evaluateAntiSpoofPolicy(IrAntispoofMode::Strict, true, true, true, true, false),
         "strict rejects an IR model failure");
}

}  // namespace

int main() {
  testModeParsing();
  testExhaustivePolicyMatrix();
  testSecurityInvariants();

  if (failures != 0) {
    std::cerr << failures << " anti-spoof policy test(s) failed\n";
    return 1;
  }
  std::cout << "All anti-spoof policy tests passed\n";
  return 0;
}
