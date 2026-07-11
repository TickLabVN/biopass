CREATE TABLE models (
    id           TEXT PRIMARY KEY,
    name         TEXT NOT NULL,
    model_type   TEXT NOT NULL CHECK (model_type IN ('detection', 'recognition', 'anti_spoofing')),
    path         TEXT NOT NULL,
    version      TEXT,
    checksum     TEXT,
    source       TEXT NOT NULL DEFAULT 'builtin',
    installed_at INTEGER NOT NULL
);
CREATE INDEX idx_models_type ON models(model_type);
