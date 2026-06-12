use serde::Serialize;
use std::fs;
use std::io::Write;
use std::path::Path;
use std::process::{Command, Stdio};

const BIOPASS_SECRET_PATH: &str = "/usr/lib/biopass/secret.yaml";
const BIOPASS_KEY_TEST_SCRIPT_PATH: &str = "/usr/lib/biopass/biopass_key_test.py";

#[derive(Debug, Serialize, Clone)]
pub struct VideoDeviceInfo {
    pub path: String,
    pub name: String,
    pub display_name: String,
}

#[tauri::command]
pub fn get_current_username() -> Result<String, String> {
    std::env::var("USER")
        .or_else(|_| std::env::var("USERNAME"))
        .map_err(|_| "Could not determine current username".to_string())
}

#[tauri::command]
pub fn has_configuration_lock() -> bool {
    Path::new(BIOPASS_SECRET_PATH).is_file()
}

#[tauri::command]
pub fn validate_configuration_lock_key(key: String) -> Result<bool, String> {
    if key.trim().is_empty() {
        return Ok(false);
    }

    if !Path::new(BIOPASS_KEY_TEST_SCRIPT_PATH).is_file() {
        return Err(format!(
            "Key validation script not found at {}",
            BIOPASS_KEY_TEST_SCRIPT_PATH
        ));
    }

    let mut child = Command::new(BIOPASS_KEY_TEST_SCRIPT_PATH)
        .arg("--password-stdin")
        .arg("--result-only")
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .map_err(|e| format!("Failed to start key validation script: {}", e))?;

    if let Some(mut stdin) = child.stdin.take() {
        stdin
            .write_all(key.as_bytes())
            .map_err(|e| format!("Failed writing key to validator stdin: {}", e))?;
    }

    let output = child
        .wait_with_output()
        .map_err(|e| format!("Failed waiting for key validation script: {}", e))?;

    let stdout = String::from_utf8_lossy(&output.stdout).trim().to_string();
    let stderr = String::from_utf8_lossy(&output.stderr).trim().to_string();

    if stdout.eq_ignore_ascii_case("success") {
        return Ok(true);
    }

    if stdout.eq_ignore_ascii_case("error") {
        return Ok(false);
    }

    if output.status.success() {
        return Ok(false);
    }

    Err(if stderr.is_empty() {
        "Key validation failed with unknown error".to_string()
    } else {
        format!("Key validation failed: {}", stderr)
    })
}

fn video_device_sort_key(path: &str) -> i32 {
    path.trim_start_matches("/dev/video")
        .parse::<i32>()
        .unwrap_or(-1)
}

#[tauri::command]
pub fn list_video_devices() -> Result<Vec<VideoDeviceInfo>, String> {
    let mut devices = Vec::new();
    let entries = fs::read_dir("/dev").map_err(|e| format!("Failed to read /dev: {}", e))?;

    for entry in entries {
        if let Ok(entry) = entry {
            let file_name = entry.file_name().to_string_lossy().to_string();
            if !file_name.starts_with("video") {
                continue;
            }

            let path = format!("/dev/{}", file_name);
            let name = fs::read_to_string(format!("/sys/class/video4linux/{}/name", file_name))
                .map(|value| value.trim().to_string())
                .unwrap_or_default();
            let display_name = if name.is_empty() {
                path.clone()
            } else {
                format!("{} ({})", name, path)
            };

            devices.push(VideoDeviceInfo {
                path,
                name,
                display_name,
            });
        }
    }

    // Sort naturally video0, video1, video2...
    devices.sort_by(|a, b| video_device_sort_key(&a.path).cmp(&video_device_sort_key(&b.path)));

    Ok(devices)
}
