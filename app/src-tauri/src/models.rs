use tauri::AppHandle;

use crate::db::{self, Model};

#[tauri::command]
pub fn list_models(app: AppHandle, model_type: Option<String>) -> Result<Vec<Model>, String> {
    let conn = db::open(&app)?;
    db::list_models(&conn, model_type.as_deref())
}
