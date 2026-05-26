export interface BiopassConfig {
  strategy: StrategyConfig;
  methods: MethodsConfig;
  models: ModelConfig[];
  appearance: string;
}

export interface StrategyConfig {
  debug: boolean;
  execution_mode: "sequential" | "parallel";
  order: string[];
  ignore_services: string[];
}

export interface MethodsConfig {
  face: FaceMethodConfig;
  fingerprint: FingerprintMethodConfig;
}

export interface VideoDeviceInfo {
  path: string;
  name: string;
  display_name: string;
}

export interface UnsharpMaskConfig {
  enable: boolean;
  amount: number;
}

export interface IRCaptureConfig {
  warmup_frames: number;
  capture_timeout_ms: number;
  poll_interval_ms: number;
  max_attempts: number;
  agc_sleep_ms: number;
  camera_warmup_ms: number;
}

export interface CaptureAdvancedConfig {
  width: number;
  height: number;
  preview_fps: number;
}

export interface DetectionAdvancedConfig {
  input_size: number;
  nms_iou_threshold: number;
}

export interface AntiSpoofingAdvancedConfig {
  spoof_class: number;
  combinational_mode: "all" | "any";
  debug_save_path: string;
}

export interface EnrollmentAdvancedConfig {
  capture_count: number;
}

export interface RecognitionAdvancedConfig {
  gallery_path: string;
}

export interface AuthAdvancedConfig {
  max_time_ms: number;
}

export const DEFAULT_ADVANCED_CONFIG: AdvancedConfig = {
  unsharp_mask: { enable: true, amount: 5.0 },
  ir_capture: { warmup_frames: 3, capture_timeout_ms: 5000, poll_interval_ms: 33, max_attempts: 2, agc_sleep_ms: 500, camera_warmup_ms: 0 },
  capture: { width: 640, height: 480, preview_fps: 3 },
  detection: { input_size: 640, nms_iou_threshold: 0.50 },
  anti_spoofing: { spoof_class: 1, combinational_mode: "all", debug_save_path: "" },
  enrollment: { capture_count: 1 },
  recognition: { gallery_path: "" },
  auth: { max_time_ms: 0 },
};

export interface AdvancedConfig {
  unsharp_mask: UnsharpMaskConfig;
  ir_capture: IRCaptureConfig;
  capture: CaptureAdvancedConfig;
  detection: DetectionAdvancedConfig;
  anti_spoofing: AntiSpoofingAdvancedConfig;
  enrollment: EnrollmentAdvancedConfig;
  recognition: RecognitionAdvancedConfig;
  auth: AuthAdvancedConfig;
}

export interface FaceMethodConfig {
  enable: boolean;
  retries: number;
  retry_delay: number;
  detection: {
    model: string;
    threshold: number;
  };
  recognition: {
    model: string;
    threshold: number;
  };
  anti_spoofing: {
    enable: boolean;
    model: {
      path: string;
      threshold: number;
    };
    ir_camera: string | null;
  };
  camera_device: string | null;
  advanced: AdvancedConfig;
}

export interface FingerprintMethodConfig {
  enable: boolean;
  retries: number;
  timeout: number;
  fingers: FingerConfig[];
}

export interface FingerConfig {
  name: string;
  created_at: number;
}

export interface ModelConfig {
  path: string;
  type: "detection" | "recognition" | "anti-spoofing";
}
