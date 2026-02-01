// Learn more about Tauri commands at https://tauri.app/develop/calling-rust/
mod config;

use config::{
    check_file_exists, delete_face_image, delete_voice_recording, download_model,
    get_config_path_str, get_current_username, list_face_images, list_video_devices,
    list_voice_recordings, load_config, save_config, save_face_image, save_voice_recording,
};

#[tauri::command]
fn greet(name: &str) -> String {
    format!("Hello, {}! You've been greeted from Rust!", name)
}

use tauri::Manager;

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .plugin(tauri_plugin_fs::init())
        .plugin(tauri_plugin_opener::init())
        .setup(|app| {
            #[cfg(target_os = "linux")]
            {
                use webkit2gtk::{PermissionRequestExt, WebViewExt};
                if let Some(window) = app.get_webview_window("main") {
                    let _ = window.with_webview(|webview| {
                        webview.inner().connect_permission_request(
                            |_view, request: &webkit2gtk::PermissionRequest| {
                                request.allow();
                                true
                            },
                        );
                    });
                }
            }
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            greet,
            load_config,
            save_config,
            get_config_path_str,
            get_current_username,
            save_face_image,
            save_voice_recording,
            list_face_images,
            list_voice_recordings,
            list_video_devices,
            delete_face_image,
            delete_voice_recording,
            download_model,
            check_file_exists
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
