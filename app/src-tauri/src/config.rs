use serde::{Deserialize, Deserializer, Serialize};
use serde_yaml::Value as YamlValue;
use std::fs;
use std::path::PathBuf;
use tauri::AppHandle;

use crate::paths::{get_config_dir, get_config_path, get_data_dir};

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct BiopassConfig {
    pub strategy: StrategyConfig,
    pub methods: MethodsConfig,
    pub models: Vec<ModelConfig>,
    pub appearance: String,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct StrategyConfig {
    #[serde(default)]
    pub debug: bool,
    pub execution_mode: String,
    pub order: Vec<String>,
    #[serde(default = "default_ignored_services")]
    pub ignore_services: Vec<String>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct MethodsConfig {
    pub face: FaceMethodConfig,
    pub fingerprint: FingerprintMethodConfig,
}

#[derive(Debug, Serialize, Clone)]
pub struct FaceMethodConfig {
    pub enable: bool,
    pub retries: u32,
    pub retry_delay: u32,
    pub detection: DetectionConfig,
    pub recognition: RecognitionConfig,
    pub anti_spoofing: AntiSpoofingConfig,
}

#[derive(Debug, Deserialize, Default)]
struct LegacyIRCameraConfig {
    #[serde(default)]
    pub enable: bool,
    #[serde(default)]
    pub device_id: i32,
}

#[derive(Debug, Deserialize, Default)]
struct AntiSpoofingConfigRaw {
    #[serde(default)]
    pub enable: bool,
    #[serde(default)]
    pub model: Option<YamlValue>,
    #[serde(default)]
    pub threshold: Option<f32>,
    #[serde(default)]
    pub ir_camera: Option<String>,
}

#[derive(Debug, Deserialize)]
struct FaceMethodConfigRaw {
    #[serde(default)]
    pub enable: bool,
    #[serde(default = "default_face_retries")]
    pub retries: u32,
    #[serde(default = "default_face_delay")]
    pub retry_delay: u32,
    #[serde(default)]
    pub detection: DetectionConfig,
    #[serde(default)]
    pub recognition: RecognitionConfig,
    #[serde(default)]
    pub anti_spoofing: AntiSpoofingConfigRaw,
    #[serde(default)]
    pub ir_camera: Option<LegacyIRCameraConfig>,
}

impl<'de> Deserialize<'de> for FaceMethodConfig {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        let raw = FaceMethodConfigRaw::deserialize(deserializer)?;

        let mut anti_spoofing = AntiSpoofingConfig::from_raw(raw.anti_spoofing);
        if anti_spoofing.ir_camera.is_none() {
            if let Some(legacy_ir_camera) = raw.ir_camera {
                if legacy_ir_camera.enable {
                    anti_spoofing.ir_camera =
                        Some(format!("/dev/video{}", legacy_ir_camera.device_id));
                }
            }
        }

        Ok(Self {
            enable: raw.enable,
            retries: raw.retries,
            retry_delay: raw.retry_delay,
            detection: raw.detection,
            recognition: raw.recognition,
            anti_spoofing,
        })
    }
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct DetectionConfig {
    pub model: String,
    pub threshold: f32,
}

impl Default for DetectionConfig {
    fn default() -> Self {
        Self {
            model: "models/yolov8n-face.onnx".to_string(),
            threshold: 0.8,
        }
    }
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct RecognitionConfig {
    pub model: String,
    pub threshold: f32,
}

impl Default for RecognitionConfig {
    fn default() -> Self {
        Self {
            model: "models/edgeface_s_gamma_05.onnx".to_string(),
            threshold: 0.8,
        }
    }
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct AntiSpoofingModelConfig {
    pub path: String,
    pub threshold: f32,
}

impl Default for AntiSpoofingModelConfig {
    fn default() -> Self {
        Self {
            path: "models/mobilenetv3_antispoof.onnx".to_string(),
            threshold: 0.8,
        }
    }
}

#[derive(Debug, Serialize, Clone)]
pub struct AntiSpoofingConfig {
    pub enable: bool,
    pub model: AntiSpoofingModelConfig,
    pub ir_camera: Option<String>,
}

impl AntiSpoofingConfig {
    fn from_raw(raw: AntiSpoofingConfigRaw) -> Self {
        let mut model = AntiSpoofingModelConfig::default();

        if let Some(model_value) = raw.model {
            match model_value {
                YamlValue::Mapping(map) => {
                    if let Some(path_value) = map.get(&YamlValue::String("path".to_string())) {
                        if let Some(path) = path_value.as_str() {
                            model.path = path.to_string();
                        }
                    }
                    if let Some(threshold_value) =
                        map.get(&YamlValue::String("threshold".to_string()))
                    {
                        if let Some(threshold) = threshold_value.as_f64() {
                            model.threshold = threshold as f32;
                        }
                    }
                }
                YamlValue::String(path) => {
                    // Backward compatibility with old schema:
                    // anti_spoofing.model: "<path>"
                    model.path = path;
                }
                _ => {}
            }
        }

        // Backward compatibility with old schema:
        // anti_spoofing.threshold: <float>
        if let Some(threshold) = raw.threshold {
            model.threshold = threshold;
        }

        Self {
            enable: raw.enable,
            model,
            ir_camera: raw.ir_camera,
        }
    }
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct FingerprintMethodConfig {
    pub enable: bool,
    #[serde(default = "default_fingerprint_retries")]
    pub retries: u32,
    // TODO: rename the actual field in config from "retry_delay" to "timeout" for clarity
    #[serde(default = "default_fingerprint_timeout", alias = "retry_delay")]
    pub timeout: u32,
    #[serde(default)]
    pub fingers: Vec<FingerConfig>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct FingerConfig {
    pub name: String,
    pub created_at: u64,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct ModelConfig {
    pub path: String,
    #[serde(rename = "type")]
    pub model_type: String,
}

fn default_face_retries() -> u32 {
    5
}
fn default_face_delay() -> u32 {
    200
}
fn default_fingerprint_retries() -> u32 {
    1
}
fn default_fingerprint_timeout() -> u32 {
    5000
}

fn default_ignored_services() -> Vec<String> {
    vec!["polkit-1".to_string(), "pkexec".to_string()]
}

fn get_default_config(app: &AppHandle) -> BiopassConfig {
    let models_dir = get_data_dir(app)
        .map(|d| d.join("models"))
        .unwrap_or_else(|_| PathBuf::from("models"));

    let model_path = |name: &str| -> String { models_dir.join(name).to_string_lossy().to_string() };

    BiopassConfig {
        strategy: StrategyConfig {
            debug: false,
            execution_mode: "parallel".to_string(),
            order: vec!["face".to_string(), "fingerprint".to_string()],
            ignore_services: default_ignored_services(),
        },
        methods: MethodsConfig {
            face: FaceMethodConfig {
                enable: true,
                retries: 5,
                retry_delay: 200,
                detection: DetectionConfig {
                    model: model_path("yolov8n-face.onnx"),
                    threshold: 0.8,
                },
                recognition: RecognitionConfig {
                    model: model_path("edgeface_s_gamma_05.onnx"),
                    threshold: 0.8,
                },
                anti_spoofing: AntiSpoofingConfig {
                    enable: true,
                    model: AntiSpoofingModelConfig {
                        path: model_path("mobilenetv3_antispoof.onnx"),
                        threshold: 0.8,
                    },
                    ir_camera: None,
                },
            },
            fingerprint: FingerprintMethodConfig {
                enable: false,
                retries: 1,
                timeout: 5000,
                fingers: vec![],
            },
        },
        models: vec![
            ModelConfig {
                path: model_path("yolov8n-face.onnx"),
                model_type: "detection".to_string(),
            },
            ModelConfig {
                path: model_path("edgeface_s_gamma_05.onnx"),
                model_type: "recognition".to_string(),
            },
            ModelConfig {
                path: model_path("mobilenetv3_antispoof.onnx"),
                model_type: "anti-spoofing".to_string(),
            },
        ],
        appearance: "system".to_string(),
    }
}

#[tauri::command]
pub fn load_config(app: AppHandle) -> Result<BiopassConfig, String> {
    let config_path = get_config_path(&app)?;

    if !config_path.exists() {
        return Ok(get_default_config(&app));
    }

    let content = fs::read_to_string(&config_path)
        .map_err(|e| format!("Failed to read config file: {}", e))?;

    serde_yaml::from_str(&content).map_err(|e| format!("Failed to parse config file: {}", e))
}

#[tauri::command]
pub fn save_config(app: AppHandle, config: BiopassConfig) -> Result<(), String> {
    let config_dir = get_config_dir(&app)?;
    let config_path = get_config_path(&app)?;

    let yaml_content =
        serde_yaml::to_string(&config).map_err(|e| format!("Failed to serialize config: {}", e))?;

    if !config_dir.exists() {
        fs::create_dir_all(&config_dir)
            .map_err(|e| format!("Failed to create config directory: {}", e))?;
    }

    fs::write(&config_path, yaml_content).map_err(|e| format!("Failed to write config file: {}", e))
}
