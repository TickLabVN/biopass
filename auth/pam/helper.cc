#include <spdlog/spdlog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <CLI/CLI.hpp>
#include <iostream>
#include <memory>
#include <opencv2/opencv.hpp>

#include "auth_config.h"
#include "auth_manager.h"
#include "face_auth.h"
#include "fingerprint_auth.h"
#include "voice_auth.h"

// Included from auth/face/detection
#include "detection/face_detection.h"

int handle_crop_face(const std::string& inputPath, const std::string& outputPath,
                     const std::string& modelPath) {
  cv::Mat image = cv::imread(inputPath);
  if (image.empty()) {
    std::cerr << "Error: Could not read input image: " << inputPath << std::endl;
    return 1;
  }

  std::unique_ptr<FaceDetection> faceDetector;
  try {
    faceDetector = std::make_unique<FaceDetection>(modelPath);
  } catch (const std::exception& e) {
    std::cerr << "Error: Failed to load detection model: " << e.what() << std::endl;
    return 1;
  }

  std::vector<Detection> detectedFaces = faceDetector->inference(image);
  if (detectedFaces.empty()) {
    std::cerr << "Error: No face detected in the image" << std::endl;
    return 2;  // Special exit code for "no face detected"
  }

  cv::Mat faceCrop = detectedFaces[0].image;
  if (!cv::imwrite(outputPath, faceCrop)) {
    std::cerr << "Error: Could not save cropped image to: " << outputPath << std::endl;
    return 1;
  }

  std::cout << "Successfully cropped face and saved to: " << outputPath << std::endl;
  return 0;
}

int handle_authenticate(const std::string& username) {
  const char* pUsername = username.c_str();

  // Load configuration from file
  if (!biopass::config_exists(pUsername)) {
    // User has not configured biopass — skip this module transparently
    return 2;  // PAM_IGNORE
  }
  biopass::BiopassConfig config = biopass::load_config(pUsername);

  if (config.debug) {
    spdlog::set_level(spdlog::level::debug);
  } else {
    spdlog::set_level(spdlog::level::info);
  }

  // Create and configure AuthManager
  biopass::AuthManager manager;
  manager.set_mode(config.mode);
  manager.set_config(config.auth);

  // Add requested authentication methods
  int methods_count = 0;
  for (const auto& method_name : config.methods) {
    if (method_name == "face") {
      manager.add_method(std::make_unique<biopass::FaceAuth>(config.methods_config.face));
      methods_count++;
    } else if (method_name == "voice") {
      manager.add_method(std::make_unique<biopass::VoiceAuth>(config.methods_config.voice));
      methods_count++;
    } else if (method_name == "fingerprint") {
      manager.add_method(
          std::make_unique<biopass::FingerprintAuth>(config.methods_config.fingerprint));
      methods_count++;
    }
  }

  // If no methods are enabled, ignore this module and let PAM jump to the next one
  if (methods_count == 0) {
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

int main(int argc, char** argv) {
  CLI::App app{"Biopass Helper Tool"};
  app.require_subcommand(0, 1);

  std::string username;
  app.add_option("username", username, "Username for authentication");

  auto crop_cmd = app.add_subcommand("crop-face", "Crop a face from an image");
  std::string inputPath, outputPath, modelPath;
  crop_cmd->add_option("--input,-i", inputPath, "Input image path")->required();
  crop_cmd->add_option("--output,-o", outputPath, "Output image path")->required();
  crop_cmd->add_option("--model,-m", modelPath, "Detection model path")->required();

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  }

  if (app.got_subcommand(crop_cmd)) {
    return handle_crop_face(inputPath, outputPath, modelPath);
  }

  if (username.empty()) {
    std::cout << app.help() << std::endl;
    return 2;  // PAM_IGNORE logic / error
  }

  return handle_authenticate(username);
}
