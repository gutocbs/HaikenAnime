CREATE TABLE IF NOT EXISTS library_scans (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    root_path TEXT NOT NULL,
    started_at TEXT NOT NULL,
    finished_at TEXT,
    status TEXT NOT NULL CHECK (status IN ('running', 'succeeded', 'failed', 'interrupted')),
    observed_count INTEGER NOT NULL DEFAULT 0 CHECK (observed_count >= 0),
    diagnostic TEXT NOT NULL DEFAULT ''
);

CREATE TABLE IF NOT EXISTS local_files (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    root_path TEXT NOT NULL,
    relative_path TEXT NOT NULL,
    normalized_relative_path TEXT NOT NULL,
    file_name TEXT NOT NULL,
    extension TEXT NOT NULL,
    size_bytes INTEGER NOT NULL CHECK (size_bytes >= 0),
    modified_at TEXT NOT NULL,
    available INTEGER NOT NULL DEFAULT 1 CHECK (available IN (0, 1)),
    last_seen_scan_id INTEGER NOT NULL REFERENCES library_scans(id),
    recognition_state TEXT NOT NULL DEFAULT 'unprocessed'
        CHECK (recognition_state IN ('unprocessed', 'recognized', 'unrecognized', 'ambiguous', 'associated', 'unsupported')),
    extracted_title TEXT NOT NULL DEFAULT '',
    media_kind TEXT NOT NULL DEFAULT 'anime',
    season INTEGER,
    episode INTEGER,
    media_id INTEGER REFERENCES media(id),
    recognition_diagnostic TEXT NOT NULL DEFAULT '',
    recognized_at TEXT,
    UNIQUE(root_path, normalized_relative_path)
);
