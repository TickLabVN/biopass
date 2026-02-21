use serde::{Deserialize, Serialize};
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
    #[serde(default)]
    pub pam_enabled: bool,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct MethodsConfig {
    pub face: FaceMethodConfig,
    pub fingerprint: FingerprintMethodConfig,
    pub voice: VoiceMethodConfig,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct FaceMethodConfig {
    pub enable: bool,
    #[serde(default = "default_face_retries")]
    pub retries: u32,
    #[serde(default = "default_face_delay")]
    pub retry_delay: u32,
    pub detection: DetectionConfig,
    pub recognition: RecognitionConfig,
    pub anti_spoofing: AntiSpoofingConfig,
    pub ir_camera: IRCameraConfig,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct IRCameraConfig {
    pub enable: bool,
    pub device_id: i32,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct DetectionConfig {
    pub model: String,
    pub threshold: f32,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct RecognitionConfig {
    pub model: String,
    pub threshold: f32,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct AntiSpoofingConfig {
    pub enable: bool,
    pub model: String,
    pub threshold: f32,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct FingerprintMethodConfig {
    pub enable: bool,
    #[serde(default = "default_fingerprint_retries")]
    pub retries: u32,
    #[serde(default = "default_fingerprint_delay")]
    pub retry_delay: u32,
    #[serde(default)]
    pub fingers: Vec<FingerConfig>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct FingerConfig {
    pub name: String,
    pub created_at: u64,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct VoiceMethodConfig {
    pub enable: bool,
    #[serde(default = "default_voice_retries")]
    pub retries: u32,
    #[serde(default = "default_voice_delay")]
    pub retry_delay: u32,
    pub model: String,
    pub threshold: f32,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct ModelConfig {
    pub path: String,
    pub name: Option<String>,
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
fn default_fingerprint_delay() -> u32 {
    5000
}
fn default_voice_retries() -> u32 {
    3
}
fn default_voice_delay() -> u32 {
    500
}

fn get_default_config(app: &AppHandle) -> BiopassConfig {
    let models_dir = get_data_dir(app)
        .map(|d| d.join("models"))
        .unwrap_or_else(|_| PathBuf::from("models"));

    let model_path = |name: &str| -> String { models_dir.join(name).to_string_lossy().to_string() };

    BiopassConfig {
        strategy: StrategyConfig {
            debug: false,
            execution_mode: "sequential".to_string(),
            order: vec![
                "face".to_string(),
                "fingerprint".to_string(),
                "voice".to_string(),
            ],
            pam_enabled: false,
        },
        methods: MethodsConfig {
            face: FaceMethodConfig {
                enable: true,
                retries: 5,
                retry_delay: 200,
                detection: DetectionConfig {
                    model: model_path("yolov11n-face.torchscript"),
                    threshold: 0.8,
                },
                recognition: RecognitionConfig {
                    model: model_path("edgeface_s_gamma_05_ts.pt"),
                    threshold: 0.8,
                },
                anti_spoofing: AntiSpoofingConfig {
                    enable: true,
                    model: model_path("mobilenetv3_antispoof_ts.pt"),
                    threshold: 0.8,
                },
                ir_camera: IRCameraConfig {
                    enable: false,
                    device_id: 0,
                },
            },
            fingerprint: FingerprintMethodConfig {
                enable: false,
                retries: 1,
                retry_delay: 5000,
                fingers: vec![],
            },
            voice: VoiceMethodConfig {
                enable: false,
                retries: 3,
                retry_delay: 500,
                model: "".to_string(),
                threshold: 0.8,
            },
        },
        models: vec![
            ModelConfig {
                path: model_path("yolov11n-face.torchscript"),
                name: Some("Face Detection".to_string()),
                model_type: "detection".to_string(),
            },
            ModelConfig {
                path: model_path("edgeface_s_gamma_05_ts.pt"),
                name: Some("Face Recognition".to_string()),
                model_type: "recognition".to_string(),
            },
            ModelConfig {
                path: model_path("mobilenetv3_antispoof_ts.pt"),
                name: Some("Face Anti-spoofing".to_string()),
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

    serde_yml::from_str(&content).map_err(|e| format!("Failed to parse config file: {}", e))
}

#[tauri::command]
pub fn save_config(app: AppHandle, config: BiopassConfig) -> Result<(), String> {
    let config_dir = get_config_dir(&app)?;
    let config_path = get_config_path(&app)?;

    let yaml_content =
        serde_yml::to_string(&config).map_err(|e| format!("Failed to serialize config: {}", e))?;

    // Create directory if needed
    if !config_dir.exists() {
        fs::create_dir_all(&config_dir)
            .map_err(|e| format!("Failed to create config directory: {}", e))?;
    }

    // Write config file
    fs::write(&config_path, yaml_content).map_err(|e| format!("Failed to write config file: {}", e))
}

#[tauri::command]
pub fn get_config_path_str(app: AppHandle) -> Result<String, String> {
    Ok(get_config_path(&app)?.to_string_lossy().to_string())
}
