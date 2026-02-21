use base64::{engine::general_purpose, Engine as _};
use std::fs;
use tauri::AppHandle;

use crate::paths::get_voices_dir;

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
    fs::write(&file_path, audio_bytes).map_err(|e| format!("Failed to write audio: {}", e))?;

    Ok(file_path.to_string_lossy().to_string())
}

#[tauri::command]
pub fn list_voice_recordings(app: AppHandle) -> Result<Vec<String>, String> {
    let voices_dir = get_voices_dir(&app)?;

    if !voices_dir.exists() {
        return Ok(vec![]);
    }

    let entries =
        fs::read_dir(&voices_dir).map_err(|e| format!("Failed to read voices directory: {}", e))?;

    let mut files: Vec<String> = entries
        .filter_map(|e| e.ok())
        .filter(|e| {
            e.path()
                .extension()
                .map_or(false, |ext| ext == "webm" || ext == "wav" || ext == "mp3")
        })
        .map(|e| e.path().to_string_lossy().to_string())
        .collect();

    files.sort();
    Ok(files)
}

#[tauri::command]
pub fn delete_voice_recording(path: String) -> Result<(), String> {
    fs::remove_file(&path).map_err(|e| format!("Failed to delete file: {}", e))
}
