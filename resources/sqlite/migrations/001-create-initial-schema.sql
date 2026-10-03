CREATE TABLE IF NOT EXISTS schema_version (
    version INTEGER PRIMARY KEY
);

CREATE TABLE IF NOT EXISTS media (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    english_name TEXT,
    original_name TEXT,
    alternative_names TEXT NOT NULL DEFAULT '[]',
    total_chapters INTEGER NOT NULL DEFAULT 0,
    consumed_chapters INTEGER NOT NULL DEFAULT 0,
    next_chapter INTEGER NOT NULL DEFAULT 0,
    average_score INTEGER NOT NULL DEFAULT 0,
    personal_score INTEGER NOT NULL DEFAULT 0,
    cover_url TEXT,
    cover_medium_url TEXT,
    cover_large_url TEXT,
    cover_extra_large_url TEXT,
    synopsis TEXT,
    type INTEGER NOT NULL,
    status INTEGER NOT NULL,
    user_list_status INTEGER NOT NULL DEFAULT -1,
    local_path TEXT NOT NULL DEFAULT '',
    source_removed_at TEXT,
    season TEXT,
    season_year INTEGER,
    next_airing_episode INTEGER,
    next_airing_at INTEGER,
    anilist_url TEXT,
    external_links TEXT NOT NULL DEFAULT '[]'
);

CREATE TABLE IF NOT EXISTS anilist_pending_changes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    media_id INTEGER NOT NULL,
    field INTEGER NOT NULL,
    previous_value TEXT NOT NULL,
    new_value TEXT NOT NULL,
    created_at TEXT NOT NULL,
    local_updated_at TEXT NOT NULL,
    remote_observed_at TEXT,
    remote_version TEXT NOT NULL DEFAULT '',
    attempts INTEGER NOT NULL DEFAULT 0,
    status INTEGER NOT NULL,
    last_error TEXT NOT NULL DEFAULT '',
    FOREIGN KEY(media_id) REFERENCES media(id)
);

CREATE TABLE IF NOT EXISTS cover_cache (
    media_id INTEGER PRIMARY KEY,
    remote_url TEXT NOT NULL,
    quality TEXT NOT NULL,
    relative_path TEXT NOT NULL,
    mime_type TEXT NOT NULL,
    byte_size INTEGER NOT NULL,
    etag TEXT NOT NULL DEFAULT '',
    last_modified TEXT NOT NULL DEFAULT '',
    validated_at TEXT NOT NULL,
    FOREIGN KEY(media_id) REFERENCES media(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS user_preferences (
    id INTEGER PRIMARY KEY CHECK (id = 1),
    score_minimum REAL NOT NULL,
    score_maximum REAL NOT NULL,
    score_step REAL NOT NULL,
    cover_quality TEXT NOT NULL,
    synchronization_enabled INTEGER NOT NULL CHECK (synchronization_enabled IN (0, 1)),
    synchronization_interval_ms INTEGER NOT NULL,
    home_sort_key TEXT NOT NULL DEFAULT 'title_asc',
    card_status_presentation TEXT NOT NULL DEFAULT 'personal-list-status',
    language_key TEXT NOT NULL DEFAULT 'pt-BR',
    preferred_title_key TEXT NOT NULL DEFAULT 'romaji',
    include_adult_content INTEGER NOT NULL DEFAULT 0 CHECK (include_adult_content IN (0, 1)),
    automatic_local_file_recognition INTEGER NOT NULL DEFAULT 1 CHECK (automatic_local_file_recognition IN (0, 1))
);
