import { invoke } from "@tauri-apps/api/core";
import { toast } from "sonner";
import type { BiopassConfig } from "@/types/config";

export async function validateConfig(config: BiopassConfig): Promise<boolean> {
  const registeredModelPaths = new Set(
    (config.models || []).map((m) => m.path),
  );

  if (config.methods.face.enable) {
    if (
      !config.methods.face.detection.model ||
      !registeredModelPaths.has(config.methods.face.detection.model)
    ) {
      toast.error("Valid Face Detection model is required");
      return false;
    }
    if (
      !config.methods.face.recognition.model ||
      !registeredModelPaths.has(config.methods.face.recognition.model)
    ) {
      toast.error("Valid Face Recognition model is required");
      return false;
    }
    if (
      config.methods.face.anti_spoofing.enable &&
      (!config.methods.face.anti_spoofing.model ||
        !registeredModelPaths.has(config.methods.face.anti_spoofing.model))
    ) {
      toast.error("Valid Anti-Spoofing model is required when enabled");
      return false;
    }

    // Validate face samples
    try {
      const samples = await invoke<string[]>("list_face_images");
      if (samples.length === 0) {
        toast.error(
          "At least one face sample must be captured before enabling Face method",
        );
        return false;
      }
    } catch (err) {
      console.error("Failed to check face samples:", err);
    }
  }

  if (config.methods.voice.enable) {
    if (
      !config.methods.voice.model ||
      !registeredModelPaths.has(config.methods.voice.model)
    ) {
      toast.error("Valid Voice Recognition model is required");
      return false;
    }

    // Validate voice samples
    try {
      const samples = await invoke<string[]>("list_voice_recordings");
      if (samples.length === 0) {
        toast.error(
          "At least one voice recording must be captured before enabling Voice method",
        );
        return false;
      }
    } catch (err) {
      console.error("Failed to check voice samples:", err);
    }
  }

  // Check for missing model files
  const modelsToCheck: string[] = [];
  if (config.methods.face.enable) {
    if (config.methods.face.detection.model)
      modelsToCheck.push(config.methods.face.detection.model);
    if (config.methods.face.recognition.model)
      modelsToCheck.push(config.methods.face.recognition.model);
    if (
      config.methods.face.anti_spoofing.enable &&
      config.methods.face.anti_spoofing.model
    ) {
      modelsToCheck.push(config.methods.face.anti_spoofing.model);
    }
  }
  if (config.methods.voice.enable && config.methods.voice.model) {
    modelsToCheck.push(config.methods.voice.model);
  }

  for (const path of modelsToCheck) {
    try {
      const exists = await invoke<boolean>("check_file_exists", { path });
      if (!exists) {
        toast.error(
          `Model file not found: ${path.split(/[\\/]/).pop()}. Please check AI Models.`,
        );
        return false;
      }
    } catch (err) {
      console.error("Status check failed:", err);
    }
  }

  return true;
}
