#include "auth_config.h"

#include <unistd.h>
#include <yaml-cpp/yaml.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

int failures = 0;
int temp_file_counter = 0;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << '\n';
    failures++;
  }
}

class TempConfig {
 public:
  explicit TempConfig(const std::string& content) {
    path_ = std::filesystem::temp_directory_path() /
            ("biopass-auth-config-test-" + std::to_string(getpid()) + "-" +
             std::to_string(temp_file_counter++) + ".yaml");
    write(content);
  }

  ~TempConfig() {
    std::error_code error;
    std::filesystem::remove(path_, error);
  }

  const std::string path() const { return path_.string(); }

  std::string read() const {
    std::ifstream input(path_);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
  }

 private:
  void write(const std::string& content) {
    std::ofstream output(path_);
    output << content;
  }

  std::filesystem::path path_;
};

std::string configWithAntiSpoofing(const std::string& anti_spoofing) {
  return "methods:\n"
         "  face:\n"
         "    anti_spoofing:\n" +
         anti_spoofing;
}

void testModeParsing() {
  {
    TempConfig file(configWithAntiSpoofing("      ir_antispoof_mode: strict\n"));
    const auto config = biopass::readConfigFile(file.path());
    expect(config.methods.face.anti_spoofing.ir_antispoof_mode == "strict",
           "C++ config reads strict mode");
  }
  {
    TempConfig file(configWithAntiSpoofing("      ir_antispoof_mode: experimental\n"));
    const auto config = biopass::readConfigFile(file.path());
    expect(config.methods.face.anti_spoofing.ir_antispoof_mode == "balanced",
           "C++ config normalizes unknown mode to balanced");
  }
  {
    TempConfig file(configWithAntiSpoofing("      ir_model_hard_fail: true\n"));
    const auto config = biopass::readConfigFile(file.path());
    expect(config.methods.face.anti_spoofing.ir_antispoof_mode == "strict",
           "legacy true maps to strict");
  }
  {
    TempConfig file(configWithAntiSpoofing("      ir_model_hard_fail: false\n"));
    const auto config = biopass::readConfigFile(file.path());
    expect(config.methods.face.anti_spoofing.ir_antispoof_mode == "balanced",
           "legacy false maps to balanced");
  }
  {
    TempConfig file(
        configWithAntiSpoofing("      ir_antispoof_mode: balanced\n"
                               "      ir_model_hard_fail: true\n"));
    const auto config = biopass::readConfigFile(file.path());
    expect(config.methods.face.anti_spoofing.ir_antispoof_mode == "balanced",
           "explicit mode takes precedence over legacy bool");
  }
}

void testLegacyMigration() {
  TempConfig file(
      "methods:\n"
      "  face:\n"
      "    anti_spoofing:\n"
      "      enable: true\n"
      "      model: legacy-antispoof.onnx\n"
      "      threshold: 0.55\n"
      "      ir_model_hard_fail: true\n"
      "    ir_camera:\n"
      "      enable: true\n"
      "      device_id: 7\n");

  std::string error;
  expect(biopass::migrateConfigFile(file.path(), &error),
         "legacy config migration succeeds: " + error);

  const YAML::Node yaml = YAML::LoadFile(file.path());
  const YAML::Node face = yaml["methods"]["face"];
  const YAML::Node anti = face["anti_spoofing"];

  expect(anti["model"].IsMap(), "legacy scalar model becomes a model map");
  expect(anti["model"]["path"].as<std::string>() == "legacy-antispoof.onnx",
         "migration preserves model path");
  expect(std::fabs(anti["model"]["threshold"].as<float>() - 0.55f) < 0.0001f,
         "migration preserves threshold");
  expect(anti["ir_camera"].as<std::string>() == "/dev/video7",
         "migration converts legacy IR device id");
  expect(anti["ir_warmup_delay_ms"].as<int>() == 150, "migration writes the warmup default");
  expect(std::fabs(anti["ir_min_face_area_ratio"].as<float>() - 0.08f) < 0.0001f,
         "migration writes the face-area default");
  expect(anti["ir_antispoof_mode"].as<std::string>() == "strict",
         "legacy hard-fail true migrates to strict");
  expect(!anti["threshold"], "migration removes legacy top-level threshold");
  expect(!anti["ir_model_hard_fail"], "migration removes legacy hard-fail key");
  expect(!face["ir_camera"], "migration removes legacy face.ir_camera block");
}

void testExplicitModeMigrationPrecedence() {
  TempConfig file(
      "methods:\n"
      "  face:\n"
      "    anti_spoofing:\n"
      "      enable: true\n"
      "      model:\n"
      "        path: model.onnx\n"
      "        threshold: 0.5\n"
      "      ir_camera: /dev/video2\n"
      "      ir_warmup_delay_ms: 150\n"
      "      ir_min_face_area_ratio: 0.08\n"
      "      ir_antispoof_mode: balanced\n"
      "      ir_model_hard_fail: true\n");

  std::string error;
  expect(biopass::migrateConfigFile(file.path(), &error),
         "explicit-mode migration succeeds: " + error);
  const YAML::Node anti = YAML::LoadFile(file.path())["methods"]["face"]["anti_spoofing"];
  expect(anti["ir_antispoof_mode"].as<std::string>() == "balanced",
         "explicit balanced mode wins over legacy true");
  expect(!anti["ir_model_hard_fail"], "legacy hard-fail key is removed");
}

void testInvalidModeMigrationAndIdempotency() {
  TempConfig file(
      "methods:\n"
      "  face:\n"
      "    anti_spoofing:\n"
      "      enable: true\n"
      "      model:\n"
      "        path: model.onnx\n"
      "        threshold: 0.5\n"
      "      ir_camera: /dev/video2\n"
      "      ir_warmup_delay_ms: 150\n"
      "      ir_min_face_area_ratio: 0.08\n"
      "      ir_antispoof_mode: experimental\n");

  std::string error;
  expect(biopass::migrateConfigFile(file.path(), &error),
         "invalid-mode migration succeeds: " + error);
  const YAML::Node anti = YAML::LoadFile(file.path())["methods"]["face"]["anti_spoofing"];
  expect(anti["ir_antispoof_mode"].as<std::string>() == "balanced",
         "invalid mode is rewritten as balanced");

  const std::string once = file.read();
  expect(biopass::migrateConfigFile(file.path(), &error), "second migration succeeds: " + error);
  expect(file.read() == once, "modern config migration is idempotent");
}

}  // namespace

int main() {
  testModeParsing();
  testLegacyMigration();
  testExplicitModeMigrationPrecedence();
  testInvalidModeMigrationAndIdempotency();

  if (failures != 0) {
    std::cerr << failures << " auth config test(s) failed\n";
    return 1;
  }
  std::cout << "All auth config tests passed\n";
  return 0;
}
