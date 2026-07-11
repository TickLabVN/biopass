# Config & model storage schema

Biopass persists per-user state in two files under
`~/.config/com.ticklab.biopass/`:

- `config.yaml` — user-editable settings (strategy, per-method thresholds,
  which model is active for each face sub-task, appearance). Parsed
  independently by the Tauri app (`app/src-tauri/src/config.rs`, via
  `serde_yaml`) and the PAM helper (`auth/core/auth_config.cc`, via
  `yaml-cpp`) — the two parsers are hand-maintained in parallel, not
  generated from a shared schema, so any field change must be applied to
  both.
- `biopass.db` — a SQLite database holding the model catalog and fingerprint
  enrollment metadata. Only the Tauri app writes to it
  (`app/src-tauri/src/db.rs`, via `rusqlite`); the PAM helper opens it
  read-only (`auth/core/model_registry.cc`, via a vendored sqlite3
  amalgamation) purely to resolve a `model_id` to a file path at login time.

## Why models moved out of config.yaml

Earlier versions kept a flat `models: [{path, type}]` array in config.yaml
that duplicated the same three file paths already present in
`methods.face.detection.model` / `recognition.model` /
`anti_spoofing.model.path`. `download_models.sh` (run at package install)
has always fetched more model variants than config.yaml ever exposed
(`edgeface_xs_gamma_06.onnx`, `minifas_v2.onnx`), so the flat array was never
a real registry — just static config text. Model data now lives in SQLite,
and config.yaml holds a `model_id` reference into it.

## `schema_version`

`config.yaml` has a top-level `schema_version` field
(`CURRENT_SCHEMA_VERSION` in both `config.rs` and `auth_config.h`, currently
`2`). Neither side migrates an old file in place — the Tauri app writes
`schema_version` on every save, and both sides fall back to defaults
(the same as a missing/corrupt file) whenever the on-disk value doesn't
match exactly. There is deliberately no v1→v2 upgrade path; a user carrying
an old install is expected to fully uninstall/reinstall, which removes both
`config.yaml` and the app data directory. This keeps the "unreadable schema"
codepath **identical** to the existing "missing config" codepath biopass
already treats as safe: `configExists()` false ⇒ PAM_IGNORE, and a defaulted
config's `model_id`s won't resolve against a database that doesn't exist yet
either, so `ensureModelsLoaded()` reports `Unavailable` and the module falls
through to normal system password auth. Nothing about a schema mismatch can
lock a user out.

## config.yaml v2 shape

```yaml
schema_version: 2
strategy:
  debug: false
  execution_mode: parallel # "parallel" | "sequential"
  order: [face, fingerprint]
  ignore_services: [polkit-1, pkexec]
methods:
  face:
    enable: true
    retries: 5
    retry_delay: 200
    camera: null
    detection: { model_id: yolov8n-face, threshold: 0.8 }
    recognition: { model_id: edgeface-s-gamma-05, threshold: 0.8 }
    anti_spoofing:
      enable: true
      model: { model_id: mobilenetv3-antispoof, threshold: 0.8 }
      ir_camera: null
      ir_warmup_delay_ms: 300
      ir_presence_timeout_ms: 1500
  fingerprint:
    enable: false
    retries: 1
    timeout: 5000
appearance: system
```

A reference fixture lives at `app/src-tauri/testdata/config_v2.yaml` and is
exercised by `app/src-tauri/src/config.rs`'s unit tests.

Fingerprint enrollment metadata (`fingers: [{name, created_at}]` in the old
schema) and the model registry no longer appear in config.yaml at all — see
below.

## biopass.db schema

DDL source of truth: `app/src-tauri/migrations/001_initial.sql`, applied via
`rusqlite_migration` (tracked with `PRAGMA user_version`).

```sql
CREATE TABLE models (
    id           TEXT PRIMARY KEY,   -- slug referenced by config.yaml's model_id fields
    name         TEXT NOT NULL,
    model_type   TEXT NOT NULL CHECK (model_type IN ('detection','recognition','anti_spoofing')),
    path         TEXT NOT NULL,
    version      TEXT,
    checksum     TEXT,
    source       TEXT NOT NULL DEFAULT 'builtin',
    installed_at INTEGER NOT NULL
);

CREATE TABLE fingerprints (
    id         INTEGER PRIMARY KEY,
    name       TEXT NOT NULL UNIQUE,
    created_at INTEGER NOT NULL
);

-- Provisioned for future use; not read from or written to yet. Directory
-- listing of <data_dir>/faces remains the source of truth for enrolled faces.
CREATE TABLE face_enrollments (
    id         INTEGER PRIMARY KEY,
    image_path TEXT NOT NULL UNIQUE,
    created_at INTEGER NOT NULL
);
```

`db::open()` seeds the `models` table on first run by scanning
`<data_dir>/models/` for the known files `download_models.sh` places there
and inserting a row per file that actually exists on disk. There is no
`is_active` column — which model is "active" for a given sub-task is
whatever `model_id` that method's block in config.yaml points at.

## Known follow-up (not addressed by this change)

`app/src-tauri/scripts/download_models.sh`'s `migrate_config_models()`
function rewrites `model:`/`path:` YAML keys containing legacy `.onnx`
filenames. Those keys don't exist in the v2 schema (only `model_id:`), so on
a v2 config the function's regexes simply won't match anything — it's an
inert no-op post-upgrade, not a correctness hazard, but is dead code worth
deleting in a follow-up cleanup.
