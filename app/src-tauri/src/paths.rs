use std::path::PathBuf;
use tauri::{AppHandle, Manager};

pub const CONFIG_FILE: &str = "config.yaml";

pub fn get_config_dir(app: &AppHandle) -> Result<PathBuf, String> {
    app.path()
        .app_config_dir()
        .map_err(|e| format!("Failed to get config dir: {}", e))
}

pub fn get_config_path(app: &AppHandle) -> Result<PathBuf, String> {
    Ok(get_config_dir(app)?.join(CONFIG_FILE))
}

pub fn get_data_dir(app: &AppHandle) -> Result<PathBuf, String> {
    app.path()
        .app_data_dir()
        .map_err(|e| format!("Failed to get data dir: {}", e))
}

pub fn get_faces_dir(app: &AppHandle) -> Result<PathBuf, String> {
    Ok(get_data_dir(app)?.join("faces"))
}

pub fn get_voices_dir(app: &AppHandle) -> Result<PathBuf, String> {
    Ok(get_data_dir(app)?.join("voices"))
}
