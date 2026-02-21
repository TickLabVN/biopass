use std::fs;

#[tauri::command]
pub fn get_current_username() -> Result<String, String> {
    std::env::var("USER")
        .or_else(|_| std::env::var("USERNAME"))
        .map_err(|_| "Could not determine current username".to_string())
}

#[tauri::command]
pub fn list_video_devices() -> Result<Vec<String>, String> {
    let mut devices = Vec::new();
    let entries = fs::read_dir("/dev").map_err(|e| format!("Failed to read /dev: {}", e))?;

    for entry in entries {
        if let Ok(entry) = entry {
            let file_name = entry.file_name().to_string_lossy().to_string();
            if file_name.starts_with("video") {
                devices.push(format!("/dev/{}", file_name));
            }
        }
    }

    // Sort naturally video0, video1, video2...
    devices.sort_by(|a, b| {
        let a_num = a
            .trim_start_matches("/dev/video")
            .parse::<i32>()
            .unwrap_or(-1);
        let b_num = b
            .trim_start_matches("/dev/video")
            .parse::<i32>()
            .unwrap_or(-1);
        a_num.cmp(&b_num)
    });

    Ok(devices)
}
