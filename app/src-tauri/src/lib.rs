pub mod camera;
pub mod config;
pub mod face;
pub mod fingerprint;
pub mod fingerprint_ffi;
pub mod paths;
pub mod system;

use camera::CameraState;
use config::{load_config, save_config};
use face::{capture_face, delete_face, list_faces};
use fingerprint::{
    add_fingerprint, delete_fingerprint, enroll_fingerprint, fingerprint_is_available,
    list_enrolled_fingerprints, list_fingerprint_devices, remove_fingerprint,
};
use system::{get_current_username, list_video_devices};

use tauri::Manager;

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_fs::init())
        .plugin(tauri_plugin_opener::init())
        .manage(CameraState {
            preview: std::sync::Mutex::new(None),
        })
        .setup(|app| {
            #[cfg(target_os = "linux")]
            {
                use webkit2gtk::{PermissionRequestExt, SettingsExt, WebViewExt};
                if let Some(window) = app.get_webview_window("main") {
                    let _ = window.with_webview(|webview| {
                        let inner = webview.inner();
                        inner.connect_permission_request(
                            |_view, request: &webkit2gtk::PermissionRequest| {
                                request.allow();
                                true
                            },
                        );

                        if let Some(settings) = inner.settings() {
                            settings.set_enable_media_stream(true);
                            settings.set_enable_webrtc(true);
                            settings.set_enable_media(true);
                        }
                    });
                }
            }

            // Stop camera preview explicitly when window closes,
            // ensuring background thread is joined before managed state drops.
            if let Some(window) = app.get_webview_window("main") {
                let app_handle = app.handle().clone();
                window.on_window_event(move |event| {
                    if let tauri::WindowEvent::Destroyed = event {
                        if let Some(state) = app_handle.try_state::<CameraState>() {
                            if let Ok(mut preview) = state.preview.lock() {
                                if let Some(mut p) = preview.take() {
                                    p.stop();
                                }
                            }
                        }
                    }
                });
            }

            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            load_config,
            save_config,
            get_current_username,
            capture_face,
            list_faces,
            list_video_devices,
            delete_face,
            add_fingerprint,
            delete_fingerprint,
            enroll_fingerprint,
            remove_fingerprint,
            fingerprint_is_available,
            list_enrolled_fingerprints,
            list_fingerprint_devices,
            camera::start_camera_preview,
            camera::get_preview_frame,
            camera::stop_camera_preview,
            camera::capture_camera_frame,
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
