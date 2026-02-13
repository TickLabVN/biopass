use crate::config::{load_config, save_config, FingerConfig};
use crate::fingerprint_ffi::{AuthResult, FingerprintAuth};
use serde::Serialize;
use std::time::{SystemTime, UNIX_EPOCH};
use tauri::AppHandle;

#[derive(Debug, Serialize, Clone)]
pub struct FingerprintDevice {
    pub name: String,
    pub driver: String,
    pub device_id: String,
}

/// Check if fingerprint hardware is available
#[tauri::command]
pub fn fingerprint_is_available() -> Result<bool, String> {
    let auth = FingerprintAuth::new();
    Ok(auth.is_available())
}

/// List fingerprint devices (compatible with current UI)
#[tauri::command]
pub fn list_fingerprint_devices() -> Result<Vec<FingerprintDevice>, String> {
    let auth = FingerprintAuth::new();
    if auth.is_available() {
        Ok(vec![FingerprintDevice {
            name: "Default Fingerprint Reader".to_string(),
            driver: "fprintd".to_string(),
            device_id: "default".to_string(),
        }])
    } else {
        Ok(vec![])
    }
}

/// List enrolled fingerprints for the current user
#[tauri::command]
pub fn list_enrolled_fingerprints(username: String) -> Result<Vec<String>, String> {
    let auth = FingerprintAuth::new();
    auth.list_enrolled_fingers(&username)
}

/// Authenticate user with fingerprint
#[tauri::command]
pub fn authenticate_fingerprint(username: String, retries: Option<i32>) -> Result<bool, String> {
    let auth = FingerprintAuth::new();
    let retries = retries.unwrap_or(3);

    match auth.authenticate(&username, retries)? {
        AuthResult::Success => Ok(true),
        _ => Ok(false),
    }
}

/// Enroll a new fingerprint
#[tauri::command]
pub async fn enroll_fingerprint(
    app: AppHandle,
    username: String,
    finger_name: String,
) -> Result<(), String> {
    let auth = FingerprintAuth::new();

    if !auth.is_available() {
        return Err("Fingerprint device not available".to_string());
    }

    // Perform enrollment through FFI
    let success = auth.enroll(&username, &finger_name, &app)?;
    if !success {
        return Err("Failed to enroll fingerprint".to_string());
    }

    // Save to config
    let mut config = load_config(app.clone())?;

    // Check if this finger is already enrolled
    if config
        .methods
        .fingerprint
        .fingers
        .iter()
        .any(|f| f.name == finger_name)
    {
        return Err(format!("Finger {} is already in config", finger_name));
    }

    config.methods.fingerprint.fingers.push(FingerConfig {
        name: finger_name.clone(),
        created_at: SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_secs(),
    });

    save_config(app, config)?;

    Ok(())
}

/// Remove an enrolled fingerprint
#[tauri::command]
pub async fn remove_fingerprint(
    app: AppHandle,
    username: String,
    finger_name: String,
) -> Result<(), String> {
    let auth = FingerprintAuth::new();

    if !auth.is_available() {
        return Err("Fingerprint device not available".to_string());
    }

    // Remove through FFI
    let success = auth.remove_finger(&username, &finger_name)?;
    if !success {
        return Err("Failed to remove fingerprint from device".to_string());
    }

    // Remove from config
    let mut config = load_config(app.clone())?;

    let original_len = config.methods.fingerprint.fingers.len();
    config
        .methods
        .fingerprint
        .fingers
        .retain(|f| f.name != finger_name);

    if config.methods.fingerprint.fingers.len() == original_len {
        return Err(format!("Finger {} not found in config", finger_name));
    }

    save_config(app, config)?;
    Ok(())
}

/// Add fingerprint (legacy, kept for compatibility)
#[tauri::command]
pub async fn add_fingerprint(
    app: AppHandle,
    _device_id: String,
    finger: String,
) -> Result<(), String> {
    // Delegate to enroll_fingerprint
    // Get username from system
    let username = std::env::var("USER")
        .or_else(|_| std::env::var("USERNAME"))
        .unwrap_or_else(|_| "root".to_string());

    enroll_fingerprint(app, username, finger).await
}

/// Delete fingerprint (legacy, kept for compatibility)
#[tauri::command]
pub fn delete_fingerprint(app: AppHandle, finger: String) -> Result<(), String> {
    let username = std::env::var("USER")
        .or_else(|_| std::env::var("USERNAME"))
        .unwrap_or_else(|_| "root".to_string());

    // Run async function synchronously
    let rt = tokio::runtime::Runtime::new().map_err(|e| e.to_string())?;
    rt.block_on(remove_fingerprint(app, username, finger))
}
