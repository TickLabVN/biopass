use base64::{engine::general_purpose, Engine as _};
use std::fs;
use tauri::AppHandle;

use crate::config::{load_config, BiopassConfig};
use crate::paths::get_faces_dir;

#[tauri::command]
pub fn save_face_image(app: AppHandle, image_data: String) -> Result<String, String> {
    let faces_dir = get_faces_dir(&app)?;
    let app_config: BiopassConfig = load_config(app.clone())?;

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
    let temp_filename = format!("temp_face_{}.jpg", timestamp);

    let file_path = faces_dir.join(&filename);
    let temp_file_path = faces_dir.join(&temp_filename);

    // Create directory if needed
    if !faces_dir.exists() {
        fs::create_dir_all(&faces_dir)
            .map_err(|e| format!("Failed to create faces directory: {}", e))?;
    }

    // Write temp file
    fs::write(&temp_file_path, &image_bytes)
        .map_err(|e| format!("Failed to write image: {}", e))?;
    // DEBUG: Save a permanent copy to /tmp to see if the frontend is generating a valid jpeg
    let _ = fs::write("/tmp/debug_capture.jpg", &image_bytes);

    // Run cropper
    let detect_model = app_config.methods.face.detection.model;

    // Check if helper exists at the absolute path first (prod), then relative (dev)
    let helper_bin = if std::path::Path::new("/usr/local/bin/biopass-helper").exists() {
        "/usr/local/bin/biopass-helper".to_string()
    } else if std::path::Path::new("../../auth/build/pam/biopass-helper").exists() {
        // Find relative to current dir
        "../../auth/build/pam/biopass-helper".to_string()
    } else {
        "biopass-helper".to_string()
    };

    let status = std::process::Command::new(&helper_bin)
        .arg("crop-face")
        .arg("--input")
        .arg(&temp_file_path)
        .arg("--output")
        .arg(&file_path)
        .arg("--model")
        .arg(&detect_model)
        .status()
        .map_err(|e| format!("Failed to execute face cropper: {}", e))?;

    // Delete temp file
    let _ = fs::remove_file(&temp_file_path);

    if status.success() {
        Ok(file_path.to_string_lossy().to_string())
    } else if status.code() == Some(2) {
        Err(
            "No face detected in the main image. Please make sure your face is visible."
                .to_string(),
        )
    } else {
        Err(format!("Cropper failed with exit status: {}", status))
    }
}

#[tauri::command]
pub fn list_face_images(app: AppHandle) -> Result<Vec<String>, String> {
    let faces_dir = get_faces_dir(&app)?;

    if !faces_dir.exists() {
        return Ok(vec![]);
    }

    let entries =
        fs::read_dir(&faces_dir).map_err(|e| format!("Failed to read faces directory: {}", e))?;

    let mut files: Vec<String> = entries
        .filter_map(|e| e.ok())
        .filter(|e| {
            e.path()
                .extension()
                .map_or(false, |ext| ext == "jpg" || ext == "png")
        })
        .map(|e| e.path().to_string_lossy().to_string())
        .collect();

    files.sort();
    Ok(files)
}

#[tauri::command]
pub fn delete_face_image(path: String) -> Result<(), String> {
    fs::remove_file(&path).map_err(|e| format!("Failed to delete file: {}", e))
}
