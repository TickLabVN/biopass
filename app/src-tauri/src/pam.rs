use std::fs;
use std::path::PathBuf;
use tauri::AppHandle;

const PAM_CONFIG_PATH: &str = "/etc/pam.d/common-auth";
const PAM_MODULE_ENTRY: &str = "auth\t[success=2 default=ignore]\tlibfacepass_pam.so";

pub fn modify_pam_lines(lines: &mut Vec<String>, pam_enabled: bool) -> bool {
    let mut changed = false;
    let existing_index = lines.iter().position(|l| l.contains("libfacepass_pam.so"));

    if pam_enabled {
        if let Some(index) = existing_index {
            // Already present, ensure it has the correct flags
            if lines[index].trim() != PAM_MODULE_ENTRY {
                lines[index] = PAM_MODULE_ENTRY.to_string();
                changed = true;
            }
        } else {
            // Not present, insert before pam_unix.so
            let unix_index = lines.iter().position(|l| l.contains("pam_unix.so"));
            if let Some(index) = unix_index {
                lines.insert(index, PAM_MODULE_ENTRY.to_string());
                changed = true;
            } else {
                // If pam_unix.so not found, just append
                lines.push(PAM_MODULE_ENTRY.to_string());
                changed = true;
            }
        }
    } else if let Some(index) = existing_index {
        // Module present but should be disabled
        lines.remove(index);
        changed = true;
    }

    changed
}

pub fn save_pam_config_with_backup(path: &PathBuf, content: &str) -> Result<(), String> {
    let is_system_path = path.to_string_lossy().starts_with("/etc/");

    // Setup backup path
    let mut backup_path = path.clone();
    backup_path.set_extension("bak");

    if is_system_path {
        // Create a temporary file in /tmp for the new config
        let temp_dir = std::env::temp_dir();
        let temp_file = temp_dir.join("facepass_pam_config_tmp");
        fs::write(&temp_file, content)
            .map_err(|e| format!("Failed to write temporary file: {}", e))?;

        // Prepare the shell script to run as root
        let backup_cmd = format!("cp {} {}", path.display(), backup_path.display());
        let copy_cmd = format!("cp {} {}", temp_file.display(), path.display());

        let script = if path.exists() {
            format!("{} && {}", backup_cmd, copy_cmd)
        } else {
            copy_cmd
        };

        // Use a single pkexec call to backup and write the new file simultaneously
        let status = std::process::Command::new("pkexec")
            .arg("sh")
            .arg("-c")
            .arg(&script)
            .status()
            .map_err(|e| format!("Failed to execute pkexec: {}", e))?;

        // Cleanup temporary file
        let _ = fs::remove_file(&temp_file);

        if !status.success() {
            return Err(
                "Failed to write config with elevated privileges. User might have cancelled."
                    .to_string(),
            );
        }
    } else {
        // Fallback for local testing (non-system files)
        if path.exists() {
            fs::copy(path, &backup_path).map_err(|e| format!("Failed to create backup: {}", e))?;
        }

        fs::write(path, content).map_err(|e| {
            format!(
                "Failed to write PAM config: {}. Try running with sudo if targeting /etc/pam.d/",
                e
            )
        })?;
    }

    Ok(())
}

#[tauri::command]
pub async fn apply_pam_config(app: AppHandle) -> Result<(), String> {
    let config =
        crate::config::load_config(app).map_err(|e| format!("Failed to load config: {}", e))?;
    let path = PathBuf::from(PAM_CONFIG_PATH);

    if !path.exists() {
        return Err(format!(
            "PAM configuration file not found: {}",
            PAM_CONFIG_PATH
        ));
    }

    let content =
        fs::read_to_string(&path).map_err(|e| format!("Failed to read PAM config: {}", e))?;
    let mut lines: Vec<String> = content.lines().map(|s| s.to_string()).collect();

    if modify_pam_lines(&mut lines, config.strategy.pam_enabled) {
        let new_content = lines.join("\n") + "\n";
        save_pam_config_with_backup(&path, &new_content)?;
    }

    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use tempfile::tempdir;

    #[test]
    fn test_modify_pam_lines_enable() {
        let mut lines = vec![
            "# comment".to_string(),
            "auth\t[success=1 default=ignore]\tpam_unix.so nullok".to_string(),
            "auth\trequisite\tpam_deny.so".to_string(),
        ];

        let changed = modify_pam_lines(&mut lines, true);
        assert!(changed);
        assert_eq!(lines.len(), 4);
        assert_eq!(lines[1], PAM_MODULE_ENTRY);
        assert_eq!(
            lines[2],
            "auth\t[success=1 default=ignore]\tpam_unix.so nullok"
        );
    }

    #[test]
    fn test_modify_pam_lines_disable() {
        let mut lines = vec![
            "# comment".to_string(),
            PAM_MODULE_ENTRY.to_string(),
            "auth\t[success=1 default=ignore]\tpam_unix.so nullok".to_string(),
        ];

        let changed = modify_pam_lines(&mut lines, false);
        assert!(changed);
        assert_eq!(lines.len(), 2);
        assert!(!lines.iter().any(|l| l.contains("libfacepass_pam.so")));
    }

    #[test]
    fn test_save_pam_config_with_backup() {
        let dir = tempdir().unwrap();
        let file_path = dir.path().join("facepass");
        let content = "test content";

        // Initial save
        save_pam_config_with_backup(&file_path, content).unwrap();
        assert_eq!(fs::read_to_string(&file_path).unwrap(), content);

        // Update with backup
        let new_content = "new content";
        save_pam_config_with_backup(&file_path, new_content).unwrap();
        assert_eq!(fs::read_to_string(&file_path).unwrap(), new_content);

        let mut backup_path = file_path.clone();
        backup_path.set_extension("bak");
        assert!(backup_path.exists());
        assert_eq!(fs::read_to_string(&backup_path).unwrap(), content);
    }
}
