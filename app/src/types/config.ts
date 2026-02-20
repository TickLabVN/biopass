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
  pam_enabled: boolean;
}

export interface MethodsConfig {
  face: FaceMethodConfig;
  fingerprint: FingerprintMethodConfig;
  voice: VoiceMethodConfig;
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
    model: string;
    threshold: number;
  };
  ir_camera: {
    enable: boolean;
    device_id: number;
  };
}

export interface FingerprintMethodConfig {
  enable: boolean;
  retries: number;
  retry_delay: number;
  fingers: FingerConfig[];
}

export interface FingerConfig {
  name: string;
  created_at: number;
}

export interface VoiceMethodConfig {
  enable: boolean;
  retries: number;
  retry_delay: number;
  model: string;
  threshold: number;
}

export interface ModelConfig {
  path: string;
  name?: string;
  type: "detection" | "recognition" | "anti-spoofing" | "voice";
}
