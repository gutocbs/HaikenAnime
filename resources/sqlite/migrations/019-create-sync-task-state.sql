CREATE TABLE IF NOT EXISTS sync_task_state (
    task_kind TEXT NOT NULL,
    partition TEXT NOT NULL,
    cache_validity TEXT NOT NULL,
    last_succeeded_at TEXT,
    last_attempted_at TEXT,
    last_error_category TEXT NOT NULL,
    consecutive_failures INTEGER NOT NULL DEFAULT 0 CHECK (consecutive_failures >= 0),
    consecutive_immediate_retries INTEGER NOT NULL DEFAULT 0 CHECK (consecutive_immediate_retries >= 0),
    UNIQUE(task_kind, partition)
);
