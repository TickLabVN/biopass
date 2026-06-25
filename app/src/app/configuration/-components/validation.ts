import { toast } from "sonner";
import { cmd } from "@/commands";
import type { BiopassConfig } from "@/types/config";

export async function validateConfig(config: BiopassConfig): Promise<boolean> {
  const registeredModelPaths = new Set(
    (config.models || []).map((m) => m.path),
  );
  const antiSpoofing = config.methods.face.anti_spoofing;
  const antiSpoofingModelRequired =
    antiSpoofing.enable || !!antiSpoofing.ir_camera;

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
      antiSpoofingModelRequired &&
      (!antiSpoofing.model.path ||
        !registeredModelPaths.has(antiSpoofing.model.path))
    ) {
      toast.error(
        "Valid Anti-Spoofing model is required when RGB AI or IR liveness is enabled",
      );
      return false;
    }

    // Validate face samples
    try {
      const samples = await cmd.face.listImages();
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

  // Check for missing model files
  const modelsToCheck: string[] = [];
  if (config.methods.face.enable) {
    if (config.methods.face.detection.model)
      modelsToCheck.push(config.methods.face.detection.model);
    if (config.methods.face.recognition.model)
      modelsToCheck.push(config.methods.face.recognition.model);
    if (antiSpoofingModelRequired && antiSpoofing.model.path) {
      modelsToCheck.push(antiSpoofing.model.path);
    }
  }
  for (const path of modelsToCheck) {
    try {
      const exists = await cmd.file.exists(path);
      if (!exists) {
        toast.error(
          `Model file not found: ${path.split(/[\\/]/).pop()}. Please check AI Models.`,
        );
        return false;
      }
    } catch (err) {
      console.error(`Failed to check model file at ${path}:`, err);
      toast.error(
        err instanceof Error
          ? err.message
          : "Unknown error occurred while validating models",
      );
      return false;
    }
  }

  return true;
}
