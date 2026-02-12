export interface FacepassConfig {
  strategy: StrategyConfig;
  methods: MethodsConfig;
  models: ModelConfig[];
  appearance: string;
}

export interface StrategyConfig {
  execution_mode: "sequential" | "parallel";
  order: string[];
  retries: number;
  retry_delay: number;
}

export interface MethodsConfig {
  face: FaceMethodConfig;
  fingerprint: FingerprintMethodConfig;
  voice: VoiceMethodConfig;
}

export interface FaceMethodConfig {
  enable: boolean;
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
  fingers: FingerConfig[];
}

export interface FingerConfig {
  name: string;
  created_at: number;
}

export interface VoiceMethodConfig {
  enable: boolean;
  model: string;
  threshold: number;
}

export interface ModelConfig {
  path: string;
  name?: string;
  type: "face" | "voice";
}

export const defaultConfig: FacepassConfig = {
  strategy: {
    execution_mode: "sequential",
    order: ["face", "fingerprint", "voice"],
    retries: 3,
    retry_delay: 500,
  },
  methods: {
    face: {
      enable: true,
      detection: {
        model: "models/face_detection.onnx",
        threshold: 0.8,
      },
      recognition: {
        model: "models/face.onnx",
        threshold: 0.8,
      },
      anti_spoofing: {
        enable: true,
        model: "models/face_anti_spoofing.onnx",
        threshold: 0.8,
      },
      ir_camera: {
        enable: false,
        device_id: 1,
      },
    },
    fingerprint: {
      enable: true,
      fingers: [],
    },
    voice: {
      enable: true,
      model: "models/voice.onnx",
      threshold: 0.8,
    },
  },
  models: [
    { path: "models/face.onnx", name: "Onnx", type: "face" },
    { path: "models/voice.onnx", type: "voice" },
  ],
  appearance: "system",
};
