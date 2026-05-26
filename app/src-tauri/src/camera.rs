use base64::{engine::general_purpose, Engine as _};
use std::io::Read;
use std::process::{Command, Stdio};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex};
use std::thread;
use std::time::Duration;

pub struct CameraPreview {
    process: Option<std::process::Child>,
    latest_frame: Arc<Mutex<Option<Vec<u8>>>>,
    running: Arc<AtomicBool>,
    thread_handle: Option<std::thread::JoinHandle<()>>,
}

impl CameraPreview {
    pub fn start(device_path: &str) -> Result<Self, String> {
        let mut child = Command::new("ffmpeg")
            .args([
                "-hide_banner",
                "-loglevel",
                "quiet",
                "-f",
                "v4l2",
                "-video_size",
                "640x480",
                "-i",
                device_path,
                "-c:v",
                "mjpeg",
                "-q:v",
                "5",
                "-f",
                "image2pipe",
                "-",
            ])
            .stdout(Stdio::piped())
            .stderr(Stdio::null())
            .spawn()
            .map_err(|e| format!("Failed to start camera: {}. Is ffmpeg installed?", e))?;

        let stdout = child
            .stdout
            .take()
            .ok_or("Failed to capture camera output")?;

        let latest_frame: Arc<Mutex<Option<Vec<u8>>>> = Arc::new(Mutex::new(None));
        let running = Arc::new(AtomicBool::new(true));

        let frame_ref = latest_frame.clone();
        let running_ref = running.clone();

        let handle = thread::spawn(move || {
            let mut reader = std::io::BufReader::new(stdout);
            let mut buf = Vec::new();

            while running_ref.load(Ordering::Relaxed) {
                let mut chunk = vec![0u8; 65536];
                match reader.read(&mut chunk) {
                    Ok(0) => break,
                    Ok(n) => {
                        buf.extend_from_slice(&chunk[..n]);
                        let mut processed = 0;
                        let mut i = 0;
                        while i + 1 < buf.len() {
                            if buf[i] == 0xFF && buf[i + 1] == 0xD8 {
                                let start = i;
                                let mut j = start + 2;
                                while j + 1 < buf.len() {
                                    if buf[j] == 0xFF && buf[j + 1] == 0xD9 {
                                        j += 2;
                                        let frame = buf[start..j].to_vec();
                                        if let Ok(mut latest) = frame_ref.lock() {
                                            *latest = Some(frame);
                                        }
                                        processed = j;
                                        i = j;
                                        break;
                                    }
                                    j += 1;
                                }
                                if j >= buf.len() - 1 {
                                    break;
                                }
                            } else {
                                i += 1;
                            }
                        }
                        if processed > 0 {
                            buf.drain(0..processed);
                        } else if buf.len() > 10 * 1024 * 1024 {
                            buf.clear();
                        }
                    }
                    Err(_) => break,
                }
            }
        });

        thread::sleep(Duration::from_millis(500));

        Ok(CameraPreview {
            process: Some(child),
            latest_frame,
            running,
            thread_handle: Some(handle),
        })
    }

    pub fn get_frame_base64(&self) -> Option<String> {
        let frame = self.latest_frame.lock().ok()?;
        frame
            .as_ref()
            .map(|data| general_purpose::STANDARD.encode(data))
    }

    pub fn stop(&mut self) {
        self.running.store(false, Ordering::Relaxed);
        if let Some(ref mut child) = self.process {
            let _ = child.kill();
            let _ = child.wait();
        }
        if let Some(handle) = self.thread_handle.take() {
            let _ = handle.join();
        }
    }
}

impl Drop for CameraPreview {
    fn drop(&mut self) {
        self.stop();
    }
}

pub struct CameraState {
    pub preview: Mutex<Option<CameraPreview>>,
}

#[tauri::command]
pub fn start_camera_preview(
    device_path: String,
    state: tauri::State<'_, CameraState>,
) -> Result<(), String> {
    let mut preview = state.preview.lock().map_err(|e| e.to_string())?;
    if let Some(mut old) = preview.take() {
        old.stop();
    }
    *preview = Some(CameraPreview::start(&device_path)?);
    Ok(())
}

#[tauri::command]
pub fn get_preview_frame(state: tauri::State<'_, CameraState>) -> Result<Option<String>, String> {
    let preview = state.preview.lock().map_err(|e| e.to_string())?;
    Ok(preview.as_ref().and_then(|p| p.get_frame_base64()))
}

#[tauri::command]
pub fn stop_camera_preview(state: tauri::State<'_, CameraState>) -> Result<(), String> {
    let mut preview = state.preview.lock().map_err(|e| e.to_string())?;
    if let Some(mut p) = preview.take() {
        p.stop();
    }
    Ok(())
}

#[tauri::command]
pub fn capture_camera_frame(device_path: String) -> Result<String, String> {
    let output = Command::new("ffmpeg")
        .args([
            "-hide_banner",
            "-loglevel",
            "quiet",
            "-f",
            "v4l2",
            "-video_size",
            "640x480",
            "-i",
            &device_path,
            "-frames:v",
            "1",
            "-c:v",
            "mjpeg",
            "-q:v",
            "5",
            "-f",
            "image2pipe",
            "-",
        ])
        .output()
        .map_err(|e| format!("Failed to run ffmpeg: {}. Is ffmpeg installed?", e))?;

    if !output.status.success() {
        let stderr = String::from_utf8_lossy(&output.stderr);
        return Err(format!("Camera capture failed: {}", stderr.trim()));
    }

    if output.stdout.is_empty() {
        return Err("Camera returned an empty frame".to_string());
    }

    Ok(general_purpose::STANDARD.encode(&output.stdout))
}
