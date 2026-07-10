import { toast } from "sonner";
import { z } from "zod";
import { cmd } from "@/commands";
import type { BiopassConfig } from "@/types/config";

const thresholdSchema = z
  .number("Threshold must be a number")
  .min(0, "Threshold must be at least 0%")
  .max(1, "Threshold must be at most 100%");

const modelSchema = z.object({
  path: z.string(),
  type: z.enum(["detection", "recognition", "anti-spoofing"]),
});

export const biopassConfigSchema = z
  .object({
    strategy: z.object({
      debug: z.boolean(),
      execution_mode: z.enum(["sequential", "parallel"]),
      order: z.array(z.string()),
      ignore_services: z.array(z.string()),
    }),
    methods: z.object({
      face: z.object({
        enable: z.boolean(),
        camera: z.string().nullable(),
        retries: z
          .number("Max retries must be a number")
          .int("Max retries must be a whole number")
          .min(1, "Max retries must be at least 1"),
        retry_delay: z
          .number("Retry delay must be a number")
          .int("Retry delay must be a whole number")
          .min(1, "Retry delay must be at least 1 ms")
          .max(5000, "Retry delay must be at most 5000 ms"),
        detection: z.object({
          model: z.string(),
          threshold: thresholdSchema,
        }),
        recognition: z.object({
          model: z.string(),
          threshold: thresholdSchema,
        }),
        anti_spoofing: z.object({
          enable: z.boolean(),
          model: z.object({
            path: z.string(),
            threshold: thresholdSchema,
          }),
          ir_camera: z.string().nullable(),
          ir_warmup_delay_ms: z
            .number("IR warmup delay must be a number")
            .int("IR warmup delay must be a whole number")
            .min(0, "IR warmup delay must be at least 0 ms"),
          ir_presence_timeout_ms: z
            .number("IR presence timeout must be a number")
            .int("IR presence timeout must be a whole number")
            .min(0, "IR presence timeout must be at least 0 ms"),
        }),
      }),
      fingerprint: z.object({
        enable: z.boolean(),
        retries: z
          .number("Max retries must be a number")
          .int("Max retries must be a whole number")
          .min(1, "Max retries must be at least 1"),
        timeout: z
          .number("Timeout must be a number")
          .int("Timeout must be a whole number")
          .min(1, "Timeout must be at least 1 ms")
          .max(5000, "Timeout must be at most 5000 ms"),
        fingers: z.array(
          z.object({
            name: z.string().min(1),
            created_at: z.number().int(),
          }),
        ),
      }),
    }),
    models: z.array(modelSchema),
    appearance: z.string(),
  })
  .superRefine((config, ctx) => {
    const registeredModelPaths = new Set(config.models.map((m) => m.path));

    if (!config.methods.face.enable) {
      return;
    }

    if (!registeredModelPaths.has(config.methods.face.detection.model)) {
      ctx.addIssue({
        code: "custom",
        message: "Valid Face Detection model is required",
        path: ["methods", "face", "detection", "model"],
      });
    }

    if (!registeredModelPaths.has(config.methods.face.recognition.model)) {
      ctx.addIssue({
        code: "custom",
        message: "Valid Face Recognition model is required",
        path: ["methods", "face", "recognition", "model"],
      });
    }

    if (
      config.methods.face.anti_spoofing.enable &&
      !registeredModelPaths.has(config.methods.face.anti_spoofing.model.path)
    ) {
      ctx.addIssue({
        code: "custom",
        message: "Valid Anti-Spoofing model is required when enabled",
        path: ["methods", "face", "anti_spoofing", "model", "path"],
      });
    }
  });

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
      (!config.methods.face.anti_spoofing.model.path ||
        !registeredModelPaths.has(config.methods.face.anti_spoofing.model.path))
    ) {
      toast.error("Valid Anti-Spoofing model is required when enabled");
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
    if (
      config.methods.face.anti_spoofing.enable &&
      config.methods.face.anti_spoofing.model.path
    ) {
      modelsToCheck.push(config.methods.face.anti_spoofing.model.path);
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
