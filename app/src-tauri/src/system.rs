use serde::Serialize;
use std::fs;
use std::io::Write;
use std::path::Path;
use std::process::{Command, Stdio};
use std::time::Duration;

#[cfg(unix)]
use std::os::unix::fs::PermissionsExt;

use wait_timeout::ChildExt;

const BIOPASS_SECRET_PATH: &str = "/usr/lib/biopass/secret.yaml";
const BIOPASS_KEY_TEST_SCRIPT_PATH: &str = "/usr/lib/biopass/biopass_key_test.py";
const BIOPASS_KEY_VALIDATION_TIMEOUT: Duration = Duration::from_secs(10);

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
pub fn has_configuration_lock() -> Result<bool, String> {
    if !Path::new(BIOPASS_SECRET_PATH).is_file() {
        return Ok(false);
    }

    ensure_key_test_script_available()?;

    Ok(true)
}

fn ensure_key_test_script_available() -> Result<(), String> {
    let script_path = Path::new(BIOPASS_KEY_TEST_SCRIPT_PATH);

    if !script_path.is_file() {
        return Err(format!(
            "Key validation script not found at {}",
            BIOPASS_KEY_TEST_SCRIPT_PATH
        ));
    }

    #[cfg(unix)]
    {
        let permissions = fs::metadata(script_path)
            .map_err(|e| format!("Failed to read key validation script metadata: {}", e))?
            .permissions();

        if permissions.mode() & 0o111 == 0 {
            return Err(format!(
                "Key validation script is not executable at {}",
                BIOPASS_KEY_TEST_SCRIPT_PATH
            ));
        }
    }

    Ok(())
}

#[tauri::command]
pub fn validate_configuration_lock_key(key: String) -> Result<bool, String> {
    if key.trim().is_empty() {
        return Ok(false);
    }

    ensure_key_test_script_available()?;

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

    let exited = child
        .wait_timeout(BIOPASS_KEY_VALIDATION_TIMEOUT)
        .map_err(|e| format!("Failed waiting for key validation script: {}", e))?;

    if exited.is_none() {
        let _ = child.kill();
        let _ = child.wait();
        return Err("Key validation timed out after 10 seconds".to_string());
    }

    let output = child
        .wait_with_output()
        .map_err(|e| format!("Failed collecting key validation output: {}", e))?;

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
