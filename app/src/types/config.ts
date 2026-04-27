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
