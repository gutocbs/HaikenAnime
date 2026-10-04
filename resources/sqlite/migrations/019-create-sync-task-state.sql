CREATE TABLE IF NOT EXISTS sync_task_state (
    task_kind TEXT NOT NULL,
    partition TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'idle',
    cache_validity TEXT NOT NULL,
    last_succeeded_at TEXT,
    last_attempted_at TEXT,
    next_run_at TEXT,
    last_error_category TEXT NOT NULL,
    safe_error_detail TEXT NOT NULL DEFAULT '',
    confirmed_page INTEGER,
    confirmed_cursor TEXT,
    priority INTEGER NOT NULL DEFAULT 0,
    generation INTEGER NOT NULL DEFAULT 0 CHECK (generation >= 0),
    consecutive_failures INTEGER NOT NULL DEFAULT 0 CHECK (consecutive_failures >= 0),
    consecutive_immediate_retries INTEGER NOT NULL DEFAULT 0 CHECK (consecutive_immediate_retries >= 0),
    UNIQUE(task_kind, partition)
);
