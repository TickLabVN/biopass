use serde::{Deserialize, Serialize};
use std::fs;
use std::path::PathBuf;
use base64::{Engine as _, engine::general_purpose};
use tauri::{AppHandle, Manager};

// XDG user directories via Tauri path API
// Config: ~/.config/facepass/config.yaml (or AppData on Windows)
// Data: ~/.local/share/facepass/faces and voices (or AppData on Windows)
const CONFIG_FILE: &str = "config.yaml";

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct FacepassConfig {
    pub strategy: StrategyConfig,
    pub methods: MethodsConfig,
    pub models: Vec<ModelConfig>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct StrategyConfig {
    pub execution_mode: String,
    pub order: Vec<String>,
    pub retries: u32,
    pub retry_delay: u32,
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
    pub detection: DetectionConfig,
    pub recognition: RecognitionConfig,
    pub anti_spoofing: AntiSpoofingConfig,
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
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct VoiceMethodConfig {
    pub enable: bool,
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

fn get_config_dir(app: &AppHandle) -> Result<PathBuf, String> {
    app.path()
        .app_config_dir()
        .map_err(|e| format!("Failed to get config dir: {}", e))
}

fn get_config_path(app: &AppHandle) -> Result<PathBuf, String> {
    Ok(get_config_dir(app)?.join(CONFIG_FILE))
}

fn get_data_dir(app: &AppHandle) -> Result<PathBuf, String> {
    app.path()
        .app_data_dir()
        .map_err(|e| format!("Failed to get data dir: {}", e))
}

fn get_faces_dir(app: &AppHandle) -> Result<PathBuf, String> {
    Ok(get_data_dir(app)?.join("faces"))
}

fn get_voices_dir(app: &AppHandle) -> Result<PathBuf, String> {
    Ok(get_data_dir(app)?.join("voices"))
}

fn get_default_config() -> FacepassConfig {
    FacepassConfig {
        strategy: StrategyConfig {
            execution_mode: "sequential".to_string(),
            order: vec!["face".to_string(), "fingerprint".to_string(), "voice".to_string()],
            retries: 3,
            retry_delay: 500,
        },
        methods: MethodsConfig {
            face: FaceMethodConfig {
                enable: true,
                detection: DetectionConfig {
                    model: "models/face_detection.onnx".to_string(),
                    threshold: 0.8,
                },
                recognition: RecognitionConfig {
                    model: "models/face.onnx".to_string(),
                    threshold: 0.8,
                },
                anti_spoofing: AntiSpoofingConfig {
                    enable: true,
                    model: "models/face_anti_spoofing.onnx".to_string(),
                    threshold: 0.8,
                },
            },
            fingerprint: FingerprintMethodConfig { enable: true },
            voice: VoiceMethodConfig {
                enable: true,
                model: "models/voice.onnx".to_string(),
                threshold: 0.8,
            },
        },
        models: vec![
            ModelConfig {
                path: "models/face.onnx".to_string(),
                name: Some("Onnx".to_string()),
                model_type: "face".to_string(),
            },
            ModelConfig {
                path: "models/fingerprint.onnx".to_string(),
                name: None,
                model_type: "fingerprint".to_string(),
            },
            ModelConfig {
                path: "models/voice.onnx".to_string(),
                name: None,
                model_type: "voice".to_string(),
            },
        ],
    }
}

#[tauri::command]
pub fn get_current_username() -> Result<String, String> {
    std::env::var("USER")
        .or_else(|_| std::env::var("USERNAME"))
        .map_err(|_| "Could not determine current username".to_string())
}

#[tauri::command]
pub fn load_config(app: AppHandle) -> Result<FacepassConfig, String> {
    let config_path = get_config_path(&app)?;

    if !config_path.exists() {
        return Ok(get_default_config());
    }

    let content = fs::read_to_string(&config_path)
        .map_err(|e| format!("Failed to read config file: {}", e))?;

    serde_yml::from_str(&content)
        .map_err(|e| format!("Failed to parse config file: {}", e))
}

#[tauri::command]
pub fn save_config(app: AppHandle, config: FacepassConfig) -> Result<(), String> {
    let config_dir = get_config_dir(&app)?;
    let config_path = get_config_path(&app)?;
    
    let yaml_content = serde_yml::to_string(&config)
        .map_err(|e| format!("Failed to serialize config: {}", e))?;

    // Create directory if needed
    if !config_dir.exists() {
        fs::create_dir_all(&config_dir)
            .map_err(|e| format!("Failed to create config directory: {}", e))?;
    }

    // Write config file
    fs::write(&config_path, yaml_content)
        .map_err(|e| format!("Failed to write config file: {}", e))
}

#[tauri::command]
pub fn get_config_path_str(app: AppHandle) -> Result<String, String> {
    Ok(get_config_path(&app)?.to_string_lossy().to_string())
}

#[tauri::command]
pub fn save_face_image(app: AppHandle, image_data: String) -> Result<String, String> {
    let faces_dir = get_faces_dir(&app)?;
    
    // Decode base64 image data
    let image_bytes = general_purpose::STANDARD
        .decode(&image_data)
        .map_err(|e| format!("Failed to decode image: {}", e))?;

    // Generate filename with timestamp
    let timestamp = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .map_err(|e| format!("Failed to get timestamp: {}", e))?
        .as_millis();
    let filename = format!("face_{}.jpg", timestamp);
    let file_path = faces_dir.join(&filename);

    // Create directory if needed
    if !faces_dir.exists() {
        fs::create_dir_all(&faces_dir)
            .map_err(|e| format!("Failed to create faces directory: {}", e))?;
    }

    // Write file
    fs::write(&file_path, image_bytes)
        .map_err(|e| format!("Failed to write image: {}", e))?;

    Ok(file_path.to_string_lossy().to_string())
}

#[tauri::command]
pub fn save_voice_recording(app: AppHandle, audio_data: String) -> Result<String, String> {
    let voices_dir = get_voices_dir(&app)?;
    
    // Decode base64 audio data
    let audio_bytes = general_purpose::STANDARD
        .decode(&audio_data)
        .map_err(|e| format!("Failed to decode audio: {}", e))?;

    // Generate filename with timestamp
    let timestamp = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .map_err(|e| format!("Failed to get timestamp: {}", e))?
        .as_millis();
    let filename = format!("voice_{}.wav", timestamp);
    let file_path = voices_dir.join(&filename);

    // Create directory if needed
    if !voices_dir.exists() {
        fs::create_dir_all(&voices_dir)
            .map_err(|e| format!("Failed to create voices directory: {}", e))?;
    }

    // Write file
    fs::write(&file_path, audio_bytes)
        .map_err(|e| format!("Failed to write audio: {}", e))?;

    Ok(file_path.to_string_lossy().to_string())
}

#[tauri::command]
pub fn list_face_images(app: AppHandle) -> Result<Vec<String>, String> {
    let faces_dir = get_faces_dir(&app)?;
    
    if !faces_dir.exists() {
        return Ok(vec![]);
    }

    let entries = fs::read_dir(&faces_dir)
        .map_err(|e| format!("Failed to read faces directory: {}", e))?;

    let mut files: Vec<String> = entries
        .filter_map(|e| e.ok())
        .filter(|e| e.path().extension().map_or(false, |ext| ext == "jpg" || ext == "png"))
        .map(|e| e.path().to_string_lossy().to_string())
        .collect();

    files.sort();
    Ok(files)
}

#[tauri::command]
pub fn list_voice_recordings(app: AppHandle) -> Result<Vec<String>, String> {
    let voices_dir = get_voices_dir(&app)?;
    
    if !voices_dir.exists() {
        return Ok(vec![]);
    }

    let entries = fs::read_dir(&voices_dir)
        .map_err(|e| format!("Failed to read voices directory: {}", e))?;

    let mut files: Vec<String> = entries
        .filter_map(|e| e.ok())
        .filter(|e| e.path().extension().map_or(false, |ext| ext == "webm" || ext == "wav" || ext == "mp3"))
        .map(|e| e.path().to_string_lossy().to_string())
        .collect();

    files.sort();
    Ok(files)
}

#[tauri::command]
pub fn delete_face_image(path: String) -> Result<(), String> {
    fs::remove_file(&path)
        .map_err(|e| format!("Failed to delete file: {}", e))
}

#[tauri::command]
pub fn delete_voice_recording(path: String) -> Result<(), String> {
    fs::remove_file(&path)
        .map_err(|e| format!("Failed to delete file: {}", e))
}
