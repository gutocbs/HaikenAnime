CREATE TABLE IF NOT EXISTS recognition_record (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    plugin_id TEXT NOT NULL CHECK (length(trim(plugin_id)) > 0),
    session_key TEXT NOT NULL CHECK (length(trim(session_key)) > 0),
    user_id TEXT NOT NULL DEFAULT '',
    device_id TEXT NOT NULL DEFAULT '',
    deduplication_key TEXT NOT NULL UNIQUE,
    title TEXT NOT NULL DEFAULT '',
    media_id INTEGER,
    season INTEGER,
    episode INTEGER NOT NULL CHECK (episode >= 1),
    confidence REAL NOT NULL CHECK (confidence >= 0 AND confidence <= 1),
    correction_title TEXT,
    correction_media_id INTEGER,
    correction_season INTEGER,
    correction_episode INTEGER CHECK (correction_episode IS NULL OR correction_episode >= 1),
    correction_confidence REAL,
    status TEXT NOT NULL,
    diagnostic TEXT NOT NULL DEFAULT '',
    sync_state TEXT NOT NULL DEFAULT 'not-required',
    sync_error TEXT NOT NULL DEFAULT '',
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS recognition_record_status_id ON recognition_record (status, id);
CREATE INDEX IF NOT EXISTS recognition_record_plugin_id ON recognition_record (plugin_id, id);
