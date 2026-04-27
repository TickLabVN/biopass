#include <spdlog/spdlog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>

#include <CLI/CLI.hpp>
#include <algorithm>
#include <memory>

#include "auth_config.h"
#include "auth_manager.h"
#include "face_auth.h"
#include "fingerprint_auth.h"
#include "image_utils.h"

#include "detection/face_detection.h"

int cropFace(const std::string& inputPath, const std::string& outputPath,
                     const std::string& modelPath) {
  ImageRGB image = readImage(inputPath);
  if (image.empty()) {
    spdlog::error("Could not read input image: {}", inputPath);
    return 1;
  }

  std::unique_ptr<FaceDetection> faceDetector;
  try {
    faceDetector = std::make_unique<FaceDetection>(modelPath);
  } catch (const std::exception& e) {
    spdlog::error("Failed to load detection model: {}", e.what());
    return 1;
  }

  std::vector<Detection> detectedFaces = faceDetector->inference(image);
  if (detectedFaces.empty()) {
    spdlog::error("No face detected in the image");
    return 2;  // Special exit code for "no face detected"
  }

  ImageRGB faceCrop = detectedFaces[0].image;
  if (!saveImage(outputPath, faceCrop)) {
    spdlog::error("Could not save cropped image to: {}", outputPath);
    return 1;
  }

  spdlog::info("Successfully cropped face and saved to: {}", outputPath);
  return 0;
}

int authenticate(const std::string& username, const std::string& service) {
  const char* pUsername = username.c_str();

  // Load configuration from file
  if (!biopass::configExists(pUsername)) {
    // User has not configured biopass — skip this module transparently
    return 2;  // PAM_IGNORE
  }
  biopass::BiopassConfig config = biopass::readConfig(pUsername);

  if (!service.empty() &&
      std::find(config.strategy.ignore_services.begin(), config.strategy.ignore_services.end(),
                service) != config.strategy.ignore_services.end()) {
    return 2;  // PAM_IGNORE
  }

  if (config.strategy.debug) {
    spdlog::set_level(spdlog::level::debug);
  } else {
    spdlog::set_level(spdlog::level::off);
  }

  biopass::AuthConfig runtime_config;
  runtime_config.debug = config.strategy.debug;
  runtime_config.antispoof =
      config.methods.face.anti_spoofing.enable ||
      (config.methods.face.anti_spoofing.ir_camera.has_value() &&
       !config.methods.face.anti_spoofing.ir_camera->empty());

  // Create and configure AuthManager
  biopass::AuthManager manager;
  manager.setMode(config.strategy.execution_mode == "sequential"
                      ? biopass::ExecutionMode::Sequential
                      : biopass::ExecutionMode::Parallel);
  manager.setConfig(runtime_config);

  // Add requested authentication methods
  int numOfMethods = 0;
  for (const auto& method_name : config.strategy.order) {
    if (method_name == "face" && config.methods.face.enable) {
      manager.addMethod(std::make_unique<biopass::FaceAuth>(config.methods.face));
      numOfMethods++;
    } else if (method_name == "fingerprint" && config.methods.fingerprint.enable) {
      manager.addMethod(std::make_unique<biopass::FingerprintAuth>(config.methods.fingerprint));
      numOfMethods++;
    }
  }

  // If no methods are enabled, ignore this module and let PAM jump to the next one
  if (numOfMethods == 0) {
    return 2;  // PAM_IGNORE
  }

  // Authenticate
  int retval = manager.authenticate(pUsername);

  if (retval == 0 /* PAM_SUCCESS is usually 0 */) {
    return 0;  // PAM_SUCCESS
  } else {
    return 1;  // PAM_AUTH_ERR
  }
}

int migrateConfig(const std::string& username) {
  if (getpwnam(username.c_str()) == nullptr) {
    spdlog::error("User '{}' not found", username);
    return 1;
  }

  std::string error;
  if (!biopass::migrateConfigSchema(username, &error)) {
    spdlog::error("Failed to migrate config schema: {}", error);
    return 1;
  }

  spdlog::info("Biopass: Config migration completed for user '{}'", username);
  return 0;
}

int main(int argc, char** argv) {
  CLI::App app{"Biopass Helper Tool"};
  app.require_subcommand(1, 1);

  auto crop_cmd = app.add_subcommand("crop-face", "Crop a face from an image");
  std::string inputPath, outputPath, modelPath;
  crop_cmd->add_option("--input,-i", inputPath, "Input image path")->required();
  crop_cmd->add_option("--output,-o", outputPath, "Output image path")->required();
  crop_cmd->add_option("--model,-m", modelPath, "Detection model path")->required();

  std::string username;
  std::string pamService;
  auto auth_cmd = app.add_subcommand("auth", "Authenticate a user with Biopass");
  auth_cmd->add_option("--username,-u", username, "Username for authentication")->required();
  auth_cmd->add_option("--service,-s", pamService, "PAM service name");

  std::string migrateUsername;
  auto migrate_cmd =
      app.add_subcommand("migrate", "Migration tool for applying schema changes (run once after updates)");
  migrate_cmd->add_option("--username,-u", migrateUsername, "Username to migrate")->required();
  migrate_cmd->group("");

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  }

  if (app.got_subcommand(crop_cmd)) {
    return cropFace(inputPath, outputPath, modelPath);
  }

  if (app.got_subcommand(auth_cmd)) {
    if (username.empty()) {
      spdlog::info("{}", app.help());
      return 2;  // PAM_IGNORE logic / error
    }
    return authenticate(username, pamService);
  }

  if (app.got_subcommand(migrate_cmd)) {
    if (migrateUsername.empty()) {
      spdlog::info("{}", app.help());
      return 1;
    }
    return migrateConfig(migrateUsername);
  }

  spdlog::error("No valid subcommand provided");
  return 1;
}
