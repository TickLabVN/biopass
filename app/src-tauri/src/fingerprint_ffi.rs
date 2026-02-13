use std::ffi::{CStr, CString};
use std::os::raw::c_char;

// FFI bindings to the C fingerprint authentication library

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct FingerprintAuthConfig {
    pub retries: i32,
}

#[link(name = "facepass_fingerprint")]
extern "C" {
    // Initialization
    fn fingerprint_auth_new() -> *mut std::ffi::c_void;
    fn fingerprint_auth_free(auth: *mut std::ffi::c_void);

    // Availability check
    fn fingerprint_is_available(auth: *mut std::ffi::c_void) -> bool;

    // Authentication
    fn fingerprint_authenticate(
        auth: *mut std::ffi::c_void,
        username: *const c_char,
        config: FingerprintAuthConfig,
    ) -> i32;

    // List enrolled fingers
    fn fingerprint_list_enrolled_fingers(
        auth: *mut std::ffi::c_void,
        username: *const c_char,
        count: *mut i32,
    ) -> *mut *mut c_char;

    fn fingerprint_free_string_array(array: *mut *mut c_char, count: i32);

    // Enroll
    fn fingerprint_enroll(
        auth: *mut std::ffi::c_void,
        username: *const c_char,
        finger_name: *const c_char,
        callback: Option<
            unsafe extern "C" fn(
                done: bool,
                status: *const c_char,
                user_data: *mut std::ffi::c_void,
            ),
        >,
        user_data: *mut std::ffi::c_void,
    ) -> bool;

    // Remove finger
    fn fingerprint_remove_finger(
        auth: *mut std::ffi::c_void,
        username: *const c_char,
        finger_name: *const c_char,
    ) -> bool;
}

/// Authentication result enum
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AuthResult {
    Success,
    Failure,
    Unavailable,
    Retry,
    Unknown,
}

impl From<i32> for AuthResult {
    fn from(value: i32) -> Self {
        match value {
            0 => AuthResult::Success,
            1 => AuthResult::Failure,
            2 => AuthResult::Unavailable,
            3 => AuthResult::Retry,
            _ => AuthResult::Unknown,
        }
    }
}

/// Safe Rust wrapper for fingerprint authentication
pub struct FingerprintAuth {
    inner: *mut std::ffi::c_void,
}

impl FingerprintAuth {
    /// Create a new fingerprint authentication instance
    pub fn new() -> Self {
        let inner = unsafe { fingerprint_auth_new() };
        FingerprintAuth { inner }
    }

    /// Check if fingerprint hardware is available
    pub fn is_available(&self) -> bool {
        unsafe { fingerprint_is_available(self.inner) }
    }

    /// Authenticate a user with fingerprint
    pub fn authenticate(&self, username: &str, retries: i32) -> Result<AuthResult, String> {
        let username_c = CString::new(username).map_err(|_| "Invalid username".to_string())?;

        let config = FingerprintAuthConfig { retries };

        let result = unsafe { fingerprint_authenticate(self.inner, username_c.as_ptr(), config) };

        Ok(AuthResult::from(result))
    }

    /// List all enrolled fingerprints for a user
    pub fn list_enrolled_fingers(&self, username: &str) -> Result<Vec<String>, String> {
        let username_c = CString::new(username).map_err(|_| "Invalid username".to_string())?;

        let mut count = 0i32;
        let array = unsafe {
            fingerprint_list_enrolled_fingers(self.inner, username_c.as_ptr(), &mut count)
        };

        if array.is_null() {
            return Ok(Vec::new());
        }

        let mut result = Vec::new();

        for i in 0..count {
            let ptr = unsafe { *array.add(i as usize) };
            if !ptr.is_null() {
                let finger_name = unsafe { CStr::from_ptr(ptr) }.to_string_lossy().to_string();
                result.push(finger_name);
            }
        }

        unsafe {
            fingerprint_free_string_array(array, count);
        }

        Ok(result)
    }

    /// Enroll a new fingerprint for a user
    pub fn enroll(
        &self,
        username: &str,
        finger_name: &str,
        app_handle: &tauri::AppHandle,
    ) -> Result<bool, String> {
        let username_c = CString::new(username).map_err(|_| "Invalid username".to_string())?;
        let finger_name_c =
            CString::new(finger_name).map_err(|_| "Invalid finger name".to_string())?;

        unsafe extern "C" fn enroll_callback(
            done: bool,
            status: *const c_char,
            user_data: *mut std::ffi::c_void,
        ) {
            use tauri::Emitter;
            let app = &*(user_data as *const tauri::AppHandle);
            let status_str = CStr::from_ptr(status).to_string_lossy().into_owned();

            #[derive(serde::Serialize, Clone)]
            struct ProgressPayload {
                done: bool,
                status: String,
            }

            app.emit(
                "fingerprint-enroll-status",
                ProgressPayload {
                    done,
                    status: status_str,
                },
            )
            .ok();
        }

        let success = unsafe {
            fingerprint_enroll(
                self.inner,
                username_c.as_ptr(),
                finger_name_c.as_ptr(),
                Some(enroll_callback),
                app_handle as *const tauri::AppHandle as *mut std::ffi::c_void,
            )
        };

        Ok(success)
    }

    /// Remove an enrolled fingerprint
    pub fn remove_finger(&self, username: &str, finger_name: &str) -> Result<bool, String> {
        let username_c = CString::new(username).map_err(|_| "Invalid username".to_string())?;
        let finger_name_c =
            CString::new(finger_name).map_err(|_| "Invalid finger name".to_string())?;

        let success = unsafe {
            fingerprint_remove_finger(self.inner, username_c.as_ptr(), finger_name_c.as_ptr())
        };

        Ok(success)
    }
}

impl Default for FingerprintAuth {
    fn default() -> Self {
        Self::new()
    }
}

impl Drop for FingerprintAuth {
    fn drop(&mut self) {
        unsafe {
            fingerprint_auth_free(self.inner);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_fingerprint_auth_creation() {
        let auth = FingerprintAuth::new();
        let available = auth.is_available();
        println!("Fingerprint available: {}", available);
    }
}
