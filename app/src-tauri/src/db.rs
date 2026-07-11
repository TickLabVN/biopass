use rusqlite::{params, Connection, OptionalExtension};
use rusqlite_migration::{Migrations, M};
use serde::{Deserialize, Serialize};
use std::time::{Duration, SystemTime, UNIX_EPOCH};
use tauri::AppHandle;

use crate::paths::{get_config_dir, get_data_dir, get_db_path};

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Model {
    pub id: String,
    pub name: String,
    pub model_type: String,
    pub path: String,
    pub version: Option<String>,
    pub checksum: Option<String>,
    pub source: String,
    pub installed_at: i64,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Fingerprint {
    pub id: i64,
    pub name: String,
    pub created_at: i64,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct FaceEnrollment {
    pub id: i64,
    pub image_path: String,
    pub created_at: i64,
}

struct KnownModel {
    slug: &'static str,
    filename: &'static str,
    display_name: &'static str,
    model_type: &'static str,
}

/// Mirrors the model list downloaded by app/src-tauri/scripts/download_models.sh.
const KNOWN_MODELS: &[KnownModel] = &[
    KnownModel {
        slug: "yolov8n-face",
        filename: "yolov8n-face.onnx",
        display_name: "YOLOv8n Face",
        model_type: "detection",
    },
    KnownModel {
        slug: "edgeface-s-gamma-05",
        filename: "edgeface_s_gamma_05.onnx",
        display_name: "EdgeFace S (gamma 0.5)",
        model_type: "recognition",
    },
    KnownModel {
        slug: "edgeface-xs-gamma-06",
        filename: "edgeface_xs_gamma_06.onnx",
        display_name: "EdgeFace XS (gamma 0.6)",
        model_type: "recognition",
    },
    KnownModel {
        slug: "mobilenetv3-antispoof",
        filename: "mobilenetv3_antispoof.onnx",
        display_name: "MobileNetV3 Anti-Spoof",
        model_type: "anti_spoofing",
    },
    KnownModel {
        slug: "minifas-v2",
        filename: "minifas_v2.onnx",
        display_name: "MiniFASNet V2",
        model_type: "anti_spoofing",
    },
];

fn now_unix() -> i64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs() as i64
}

fn migrations() -> Migrations<'static> {
    Migrations::new(vec![M::up(include_str!("../migrations/001_initial.sql"))])
}

/// Opens (creating if necessary) the biopass.db sqlite database, applies any
/// pending migrations, and seeds the `models` table from the known bundled
/// models on first run.
pub fn open(app: &AppHandle) -> Result<Connection, String> {
    let config_dir = get_config_dir(app)?;
    if !config_dir.exists() {
        std::fs::create_dir_all(&config_dir)
            .map_err(|e| format!("Failed to create config directory: {}", e))?;
    }

    let db_path = get_db_path(app)?;
    let mut conn =
        Connection::open(&db_path).map_err(|e| format!("Failed to open database: {}", e))?;
    conn.busy_timeout(Duration::from_secs(2))
        .map_err(|e| format!("Failed to set database busy timeout: {}", e))?;

    migrations()
        .to_latest(&mut conn)
        .map_err(|e| format!("Failed to run database migrations: {}", e))?;

    seed_default_models(&conn, app)?;

    Ok(conn)
}

fn seed_default_models(conn: &Connection, app: &AppHandle) -> Result<(), String> {
    let count: i64 = conn
        .query_row("SELECT COUNT(*) FROM models", [], |row| row.get(0))
        .map_err(|e| format!("Failed to count models: {}", e))?;
    if count > 0 {
        return Ok(());
    }

    let models_dir = get_data_dir(app)?.join("models");
    let now = now_unix();

    for known in KNOWN_MODELS {
        let path = models_dir.join(known.filename);
        if !path.exists() {
            continue;
        }
        conn.execute(
            "INSERT OR IGNORE INTO models (id, name, model_type, path, version, checksum, source, installed_at) \
             VALUES (?1, ?2, ?3, ?4, NULL, NULL, 'builtin', ?5)",
            params![
                known.slug,
                known.display_name,
                known.model_type,
                path.to_string_lossy(),
                now
            ],
        )
        .map_err(|e| format!("Failed to seed model '{}': {}", known.slug, e))?;
    }

    Ok(())
}

fn row_to_model(row: &rusqlite::Row) -> rusqlite::Result<Model> {
    Ok(Model {
        id: row.get(0)?,
        name: row.get(1)?,
        model_type: row.get(2)?,
        path: row.get(3)?,
        version: row.get(4)?,
        checksum: row.get(5)?,
        source: row.get(6)?,
        installed_at: row.get(7)?,
    })
}

const MODEL_COLUMNS: &str = "id, name, model_type, path, version, checksum, source, installed_at";

pub fn list_models(conn: &Connection, model_type: Option<&str>) -> Result<Vec<Model>, String> {
    let query = format!(
        "SELECT {} FROM models WHERE (?1 IS NULL OR model_type = ?1) ORDER BY installed_at",
        MODEL_COLUMNS
    );
    let mut stmt = conn
        .prepare(&query)
        .map_err(|e| format!("Failed to prepare models query: {}", e))?;
    let rows = stmt
        .query_map(params![model_type], row_to_model)
        .map_err(|e| format!("Failed to query models: {}", e))?;
    rows.collect::<Result<Vec<_>, _>>()
        .map_err(|e| format!("Failed to read models: {}", e))
}

pub fn get_model(conn: &Connection, id: &str) -> Result<Option<Model>, String> {
    let query = format!("SELECT {} FROM models WHERE id = ?1", MODEL_COLUMNS);
    conn.query_row(&query, params![id], row_to_model)
        .optional()
        .map_err(|e| format!("Failed to query model '{}': {}", id, e))
}

pub fn resolve_model_path(conn: &Connection, id: &str) -> Result<Option<String>, String> {
    conn.query_row(
        "SELECT path FROM models WHERE id = ?1",
        params![id],
        |row| row.get(0),
    )
    .optional()
    .map_err(|e| format!("Failed to resolve model path for '{}': {}", id, e))
}

#[allow(clippy::too_many_arguments)]
pub fn insert_model(
    conn: &Connection,
    id: &str,
    name: &str,
    model_type: &str,
    path: &str,
    version: Option<&str>,
    checksum: Option<&str>,
    source: &str,
) -> Result<(), String> {
    conn.execute(
        "INSERT INTO models (id, name, model_type, path, version, checksum, source, installed_at) \
         VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8)",
        params![
            id,
            name,
            model_type,
            path,
            version,
            checksum,
            source,
            now_unix()
        ],
    )
    .map_err(|e| format!("Failed to insert model '{}': {}", id, e))?;
    Ok(())
}

pub fn delete_model(conn: &Connection, id: &str) -> Result<(), String> {
    conn.execute("DELETE FROM models WHERE id = ?1", params![id])
        .map_err(|e| format!("Failed to delete model '{}': {}", id, e))?;
    Ok(())
}

pub fn list_fingerprints(conn: &Connection) -> Result<Vec<Fingerprint>, String> {
    let mut stmt = conn
        .prepare("SELECT id, name, created_at FROM fingerprints ORDER BY created_at")
        .map_err(|e| format!("Failed to prepare fingerprints query: {}", e))?;
    let rows = stmt
        .query_map([], |row| {
            Ok(Fingerprint {
                id: row.get(0)?,
                name: row.get(1)?,
                created_at: row.get(2)?,
            })
        })
        .map_err(|e| format!("Failed to query fingerprints: {}", e))?;
    rows.collect::<Result<Vec<_>, _>>()
        .map_err(|e| format!("Failed to read fingerprints: {}", e))
}

pub fn insert_fingerprint(conn: &Connection, name: &str) -> Result<Fingerprint, String> {
    let created_at = now_unix();
    conn.execute(
        "INSERT INTO fingerprints (name, created_at) VALUES (?1, ?2)",
        params![name, created_at],
    )
    .map_err(|e| format!("Failed to insert fingerprint '{}': {}", name, e))?;
    Ok(Fingerprint {
        id: conn.last_insert_rowid(),
        name: name.to_string(),
        created_at,
    })
}

pub fn delete_fingerprint(conn: &Connection, name: &str) -> Result<(), String> {
    conn.execute("DELETE FROM fingerprints WHERE name = ?1", params![name])
        .map_err(|e| format!("Failed to delete fingerprint '{}': {}", name, e))?;
    Ok(())
}

// Face-enrollment rows are provisioned for future use and not wired into any
// read/write path yet; directory listing under <data_dir>/faces remains the
// source of truth for now.
#[allow(dead_code)]
pub fn list_face_enrollments(conn: &Connection) -> Result<Vec<FaceEnrollment>, String> {
    let mut stmt = conn
        .prepare("SELECT id, image_path, created_at FROM face_enrollments ORDER BY created_at")
        .map_err(|e| format!("Failed to prepare face_enrollments query: {}", e))?;
    let rows = stmt
        .query_map([], |row| {
            Ok(FaceEnrollment {
                id: row.get(0)?,
                image_path: row.get(1)?,
                created_at: row.get(2)?,
            })
        })
        .map_err(|e| format!("Failed to query face_enrollments: {}", e))?;
    rows.collect::<Result<Vec<_>, _>>()
        .map_err(|e| format!("Failed to read face_enrollments: {}", e))
}

#[allow(dead_code)]
pub fn insert_face_enrollment(conn: &Connection, image_path: &str) -> Result<(), String> {
    conn.execute(
        "INSERT INTO face_enrollments (image_path, created_at) VALUES (?1, ?2)",
        params![image_path, now_unix()],
    )
    .map_err(|e| format!("Failed to insert face enrollment '{}': {}", image_path, e))?;
    Ok(())
}

#[allow(dead_code)]
pub fn delete_face_enrollment(conn: &Connection, image_path: &str) -> Result<(), String> {
    conn.execute(
        "DELETE FROM face_enrollments WHERE image_path = ?1",
        params![image_path],
    )
    .map_err(|e| format!("Failed to delete face enrollment '{}': {}", image_path, e))?;
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    fn setup_conn() -> Connection {
        let mut conn = Connection::open_in_memory().unwrap();
        migrations().to_latest(&mut conn).unwrap();
        conn
    }

    #[test]
    fn insert_and_resolve_model() {
        let conn = setup_conn();
        insert_model(
            &conn,
            "yolov8n-face",
            "YOLOv8n Face",
            "detection",
            "/data/models/yolov8n-face.onnx",
            None,
            None,
            "builtin",
        )
        .unwrap();

        let path = resolve_model_path(&conn, "yolov8n-face").unwrap();
        assert_eq!(path.as_deref(), Some("/data/models/yolov8n-face.onnx"));

        let missing = resolve_model_path(&conn, "does-not-exist").unwrap();
        assert!(missing.is_none());
    }

    #[test]
    fn list_models_filters_by_type() {
        let conn = setup_conn();
        insert_model(
            &conn,
            "a",
            "A",
            "detection",
            "/a.onnx",
            None,
            None,
            "builtin",
        )
        .unwrap();
        insert_model(
            &conn,
            "b",
            "B",
            "recognition",
            "/b.onnx",
            None,
            None,
            "builtin",
        )
        .unwrap();

        let detection = list_models(&conn, Some("detection")).unwrap();
        assert_eq!(detection.len(), 1);
        assert_eq!(detection[0].id, "a");

        let all = list_models(&conn, None).unwrap();
        assert_eq!(all.len(), 2);
    }

    #[test]
    fn fingerprint_crud() {
        let conn = setup_conn();
        let fp = insert_fingerprint(&conn, "right-index").unwrap();
        assert_eq!(fp.name, "right-index");

        let all = list_fingerprints(&conn).unwrap();
        assert_eq!(all.len(), 1);

        delete_fingerprint(&conn, "right-index").unwrap();
        assert!(list_fingerprints(&conn).unwrap().is_empty());
    }
}
