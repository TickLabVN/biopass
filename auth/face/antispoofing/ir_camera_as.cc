#include "ir_camera_as.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <thread>

#include "auth_config.h"
#include "camera_capture.h"
#include "debug_image_io.h"
#include "face_as.h"
#include "face_detection.h"
#include "ir_liveness_analyzer.h"

namespace biopass {

IRLivenessResult checkAntispoofByIRCamera(ICameraCaptureSession* ir_camera_session,
                                          const std::string& ir_camera_path,
                                          FaceDetection* ir_det_model,
                                          FaceAntiSpoofing* ir_as_model, int warmup_delay_ms,
                                          float min_face_area_ratio, const std::string& username,
                                          bool debug_enabled) {
  if (!ir_det_model) {
    spdlog::error("FaceAuth: IR liveness check — FaceDetection model is null");
    return {};
  }
  if (!ir_as_model) {
    spdlog::error("FaceAuth: IR liveness check — FaceAntiSpoofing model is null");
    return {};
  }

  // Helper lambda: capture one IR frame from an open session or new session.
  auto capture_ir = [&]() -> ImageRGB {
    if (ir_camera_session && ir_camera_session->isOpen()) {
      return ir_camera_session->capture();
    }
    return captureImageByIRCamera(ir_camera_path, /*warmup_frames=*/5,
                                  /*timeout_ms=*/3000,
                                  /*poll_interval_ms=*/10);
  };

  // LED warm-up retry: use cheap brightness stats. Face detection is kept
  // for usable-frame selection only, so AI and IR work can overlap sooner.
  constexpr int kMaxLedWarmupRetries = 10;
  constexpr int kMinLedWarmupRetryDelayMs = 50;
  constexpr int kMaxLedWarmupRetryDelayMs = 300;
  const int led_warmup_retry_delay_ms =
      std::clamp(warmup_delay_ms, kMinLedWarmupRetryDelayMs, kMaxLedWarmupRetryDelayMs);
  constexpr float kIrFacePresentThreshold = 0.45f;
  constexpr int kRequiredUsableIrFrames = 2;
  constexpr int kMaxSelectedIrFaces = 3;
  constexpr int kMaxIrFrameSelectionAttempts = 12;
  constexpr int kIrFrameSelectionIntervalMs = 50;

  auto process_and_log_ir_frame = [&](const ImageRGB& frame,
                                      int attempt) -> ir_liveness::FrameStats {
    if (frame.empty()) {
      spdlog::debug("FaceAuth: IR frame stats (attempt {}) — frame is empty", attempt);
      return {};
    }
    const ir_liveness::FrameStats stats = ir_liveness::calculateFrameStats(frame);

    if (debug_enabled) {
      const std::string path =
          getDebugPath(username) + "/ir_raw_attempt_" + std::to_string(attempt) + ".png";
      saveImage(path, frame);
    }

    spdlog::debug(
        "FaceAuth: IR frame stats (attempt {}) — shape=({}x{}x3), "
        "dtype=uint8, min={}, max={}, mean={:.2f}, stddev={:.2f}, "
        "tile_mean_stddev={:.2f}, tile_stddev_mean={:.2f}",
        attempt, frame.width, frame.height, stats.min_val, stats.max_val, stats.mean_val,
        stats.stddev_val, stats.tile_mean_stddev, stats.tile_stddev_mean);
    return stats;
  };

  spdlog::debug(
      "FaceAuth: IR liveness check — capturing frame (LED warm-up check "
      "enabled)");
  ImageRGB ir_frame = capture_ir();
  process_and_log_ir_frame(ir_frame, 0);

  try {
    ir_liveness::FrameStats ir_stats = ir_liveness::calculateFrameStats(ir_frame);
    for (int attempt = 0; attempt < kMaxLedWarmupRetries && !ir_frame.empty(); ++attempt) {
      if (ir_liveness::isIlluminatedFrame(ir_stats)) {
        if (attempt > 0) {
          spdlog::debug(
              "FaceAuth: IR liveness check — LED ready after {} "
              "recapture(s) (mean={:.2f}, max={}, stddev={:.2f})",
              attempt, ir_stats.mean_val, ir_stats.max_val, ir_stats.stddev_val);
        }
        break;
      }

      spdlog::debug(
          "FaceAuth: IR liveness check — IR frame is not illuminated enough "
          "(mean={:.2f}, max={}, stddev={:.2f}), LED may not be ready — "
          "recapturing in {}ms (attempt {}/{})",
          ir_stats.mean_val, ir_stats.max_val, ir_stats.stddev_val, led_warmup_retry_delay_ms,
          attempt + 1, kMaxLedWarmupRetries);
      std::this_thread::sleep_for(std::chrono::milliseconds(led_warmup_retry_delay_ms));
      ir_frame = capture_ir();
      ir_stats = process_and_log_ir_frame(ir_frame, attempt + 1);
    }

    std::vector<ImageRGB> ir_faces;
    std::vector<int> ir_face_frame_indices;
    std::vector<ir_liveness::FrameStats> pulse_stats;
    ir_liveness::PulseValidationResult pulse_result;
    for (int attempt = 0; attempt < kMaxIrFrameSelectionAttempts; ++attempt) {
      ImageRGB candidate = attempt == 0 ? ir_frame : capture_ir();
      const ir_liveness::FrameStats stats = process_and_log_ir_frame(candidate, 100 + attempt);
      if (candidate.empty()) {
        continue;
      }
      pulse_stats.push_back(stats);
      const int pulse_frame_index = static_cast<int>(pulse_stats.size()) - 1;

      const bool illuminated_frame = ir_liveness::isIlluminatedFrame(stats);
      const bool dark_pulse_frame = ir_liveness::isDarkPulseFrame(stats);
      if (!illuminated_frame) {
        if (dark_pulse_frame) {
          spdlog::debug(
              "FaceAuth: IR liveness check — observed dark IR pulse frame "
              "(mean={:.2f}, max={}, stddev={:.2f}, tile_mean_stddev={:.2f})",
              stats.mean_val, stats.max_val, stats.stddev_val, stats.tile_mean_stddev);
        } else {
          spdlog::debug(
              "FaceAuth: IR liveness check — skipping non-illuminated/non-pulse IR frame "
              "(mean={:.2f}, max={}, stddev={:.2f}, tile_mean_stddev={:.2f})",
              stats.mean_val, stats.max_val, stats.stddev_val, stats.tile_mean_stddev);
        }

        pulse_result = ir_liveness::validateIlluminationPulse(pulse_stats);
        if ((int)ir_faces.size() >= kRequiredUsableIrFrames && pulse_result.passed &&
            ir_liveness::acceptedFramesBracketDarkPulse(pulse_stats, ir_face_frame_indices)) {
          break;
        }
        if (attempt + 1 < kMaxIrFrameSelectionAttempts) {
          std::this_thread::sleep_for(std::chrono::milliseconds(kIrFrameSelectionIntervalMs));
        }
        continue;
      }

      if ((int)ir_faces.size() < kMaxSelectedIrFaces) {
        const std::vector<Detection> raw = ir_det_model->inference(candidate);
        float max_conf = 0.0f;
        int best_detection_index = -1;
        for (size_t i = 0; i < raw.size(); ++i) {
          const auto& d = raw[i];
          if (d.conf > max_conf) {
            max_conf = d.conf;
            best_detection_index = static_cast<int>(i);
          }
        }

        if (best_detection_index >= 0 && max_conf >= kIrFacePresentThreshold) {
          const Detection& best_detection = raw[best_detection_index];
          const int bbox_w = best_detection.box.x2 - best_detection.box.x1;
          const int bbox_h = best_detection.box.y2 - best_detection.box.y1;
          const float frame_area =
              static_cast<float>(candidate.width) * static_cast<float>(candidate.height);
          const float bbox_area_ratio =
              frame_area > 0.0f
                  ? (static_cast<float>(bbox_w) * static_cast<float>(bbox_h)) / frame_area
                  : 0.0f;

          if (bbox_area_ratio < min_face_area_ratio) {
            spdlog::debug(
                "FaceAuth: IR liveness check — skipping IR face too small for reliable "
                "liveness (face_conf={:.4f}, bbox={}x{}, area_ratio={:.4f} < {:.4f})",
                max_conf, bbox_w, bbox_h, bbox_area_ratio, min_face_area_ratio);
          } else {
            ir_faces.push_back(best_detection.image);
            ir_face_frame_indices.push_back(pulse_frame_index);
            if (debug_enabled) {
              saveFailedFace(username, best_detection.image, "ir_selected_crop");
              saveFailedFace(username, resizeImage(best_detection.image, 128, 128),
                             "ir_model_input_128");
            }

            spdlog::debug(
                "FaceAuth: IR liveness check — accepted usable IR face {}/{} minimum "
                "(face_conf={:.4f}, bbox={}x{}, area_ratio={:.4f}, mean={:.2f}, "
                "max={}, stddev={:.2f})",
                ir_faces.size(), kRequiredUsableIrFrames, max_conf, bbox_w, bbox_h, bbox_area_ratio,
                stats.mean_val, stats.max_val, stats.stddev_val);
          }
        } else {
          spdlog::debug(
              "FaceAuth: IR liveness check — skipping IR frame with weak face "
              "confidence ({:.4f} < {:.4f})",
              max_conf, kIrFacePresentThreshold);
        }
      }

      pulse_result = ir_liveness::validateIlluminationPulse(pulse_stats);
      if ((int)ir_faces.size() >= kRequiredUsableIrFrames && pulse_result.passed &&
          ir_liveness::acceptedFramesBracketDarkPulse(pulse_stats, ir_face_frame_indices)) {
        break;
      }

      if (attempt + 1 < kMaxIrFrameSelectionAttempts) {
        std::this_thread::sleep_for(std::chrono::milliseconds(kIrFrameSelectionIntervalMs));
      }
    }

    if (ir_faces.size() < 2) {
      spdlog::error(
          "FaceAuth: IR liveness check — only {} usable IR face(s) captured "
          "from '{}' (required >= 2)",
          ir_faces.size(), ir_camera_path);
      return {};
    }

    if (!ir_liveness::acceptedFramesBracketDarkPulse(pulse_stats, ir_face_frame_indices)) {
      spdlog::error(
          "FaceAuth: IR liveness check — usable IR face frames did not bracket a valid "
          "dark pulse frame (usable_faces={}, observed_frames={})",
          ir_faces.size(), pulse_stats.size());
      return {};
    }

    if (!pulse_result.passed) {
      pulse_result = ir_liveness::validateIlluminationPulse(pulse_stats);
      spdlog::error(
          "FaceAuth: IR liveness check — IR illumination pulse validation failed: {} "
          "(illuminated_frames={}, dark_frames={}, transitions={}, mean_swing={:.2f})",
          pulse_result.reason, pulse_result.illuminated_frame_count, pulse_result.dark_frame_count,
          pulse_result.transition_count, pulse_result.mean_swing);
      return {};
    }

    spdlog::debug(
        "FaceAuth: IR liveness check — IR illumination pulse validated "
        "(illuminated_frames={}, dark_frames={}, transitions={}, mean_swing={:.2f}, "
        "approx_bright_dark_bright_span={}ms)",
        pulse_result.illuminated_frame_count, pulse_result.dark_frame_count,
        pulse_result.transition_count, pulse_result.mean_swing,
        pulse_result.pulse_span_frames * kIrFrameSelectionIntervalMs);

    return {true, checkAntispoofByIRCrops(ir_faces, ir_as_model, username, debug_enabled)};
  } catch (const std::exception& e) {
    spdlog::warn("FaceAuth: IR liveness check — frame selection error: {}", e.what());
    return {};
  }
}

bool checkAntispoofByIRCrops(const std::vector<ImageRGB>& ir_faces, FaceAntiSpoofing* face_as,
                             const std::string& username, bool debug) {
  if (!face_as) {
    spdlog::error("FaceAuth: IR liveness check — anti-spoofing model is null");
    return false;
  }

  const float antispoof_threshold = face_as->getThreshold();

  spdlog::debug("FaceAuth: IR liveness check | preselected_faces={} antispoof_threshold={:.3f}",
                ir_faces.size(), antispoof_threshold);

  if (ir_faces.empty()) {
    spdlog::error("FaceAuth: IR liveness check — received no preselected face crops, aborting");
    return false;
  }

  int passed_faces = 0;
  int valid_faces_processed = 0;
  float real_score_sum = 0.0f;
  float spoof_score_sum = 0.0f;

  try {
    for (size_t face_idx = 0; face_idx < ir_faces.size(); ++face_idx) {
      const auto& face = ir_faces[face_idx];
      if (face.empty()) {
        spdlog::error("FaceAuth: IR liveness check — selected face {}/{} is empty/invalid",
                      face_idx + 1, ir_faces.size());
        return false;
      }
      valid_faces_processed++;

      const SpoofResult result = face_as->inference(face);
      const bool is_face_real = result.is_real;
      real_score_sum += result.real_score;
      spoof_score_sum += result.spoof_score;
      if (!is_face_real) {
        spdlog::warn(
            "FaceAuth: IR liveness check [face {}/{}] — FAILED (crop={}x{}, real={:.4f}, "
            "spoof={:.4f}, threshold={:.3f})",
            face_idx + 1, ir_faces.size(), face.width, face.height, result.real_score,
            result.spoof_score, antispoof_threshold);
        if (debug) {
          saveFailedFace(username, face, "ir_spoof_face_" + std::to_string(face_idx));
        }
      } else {
        spdlog::debug(
            "FaceAuth: IR liveness check [face {}/{}] — PASSED (crop={}x{}, real={:.4f}, "
            "spoof={:.4f}, threshold={:.3f})",
            face_idx + 1, ir_faces.size(), face.width, face.height, result.real_score,
            result.spoof_score, antispoof_threshold);
        passed_faces++;
      }
    }

    if (valid_faces_processed == 0) {
      spdlog::error("FaceAuth: IR liveness check — all preselected face crops were empty/invalid");
      return false;
    }

    const float avg_real_score = real_score_sum / static_cast<float>(valid_faces_processed);
    const float avg_spoof_score = spoof_score_sum / static_cast<float>(valid_faces_processed);
    const bool passed = passed_faces == valid_faces_processed;

    if (passed) {
      spdlog::debug(
          "FaceAuth: IR liveness check AGGREGATE PASSED — {}/{} selected crop(s) passed, "
          "avg_real={:.4f}, avg_spoof={:.4f}, threshold={:.3f}",
          passed_faces, valid_faces_processed, avg_real_score, avg_spoof_score,
          antispoof_threshold);
    } else {
      spdlog::warn(
          "FaceAuth: IR liveness check AGGREGATE FAILED — {}/{} selected crop(s) passed, "
          "avg_real={:.4f}, avg_spoof={:.4f}, threshold={:.3f}",
          passed_faces, valid_faces_processed, avg_real_score, avg_spoof_score,
          antispoof_threshold);
    }
    return passed;
  } catch (const std::exception& e) {
    spdlog::error("FaceAuth: IR liveness check — exception during preselected inference: {}",
                  e.what());
    return false;
  }
}

}  // namespace biopass
