INSERT INTO sync_task_state
    (task_kind, partition, cache_validity, last_succeeded_at, last_attempted_at,
     last_error_category, consecutive_failures, consecutive_immediate_retries)
VALUES
    (:task_kind, :partition, :cache_validity, :last_succeeded_at, :last_attempted_at,
     :last_error_category, :consecutive_failures, :consecutive_immediate_retries)
ON CONFLICT(task_kind, partition) DO UPDATE SET
    cache_validity = excluded.cache_validity,
    last_succeeded_at = excluded.last_succeeded_at,
    last_attempted_at = excluded.last_attempted_at,
    last_error_category = excluded.last_error_category,
    consecutive_failures = excluded.consecutive_failures,
    consecutive_immediate_retries = excluded.consecutive_immediate_retries;
