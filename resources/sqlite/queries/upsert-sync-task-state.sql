INSERT INTO sync_task_state
    (task_kind, partition, status, cache_validity, last_succeeded_at, last_attempted_at,
     next_run_at, last_error_category, safe_error_detail, confirmed_page, confirmed_cursor,
     priority, generation, consecutive_failures, consecutive_immediate_retries)
VALUES
    (:task_kind, :partition, :status, :cache_validity, :last_succeeded_at, :last_attempted_at,
     :next_run_at, :last_error_category, :safe_error_detail, :confirmed_page, :confirmed_cursor,
     :priority, :generation, :consecutive_failures, :consecutive_immediate_retries)
ON CONFLICT(task_kind, partition) DO UPDATE SET
    status = excluded.status,
    cache_validity = excluded.cache_validity,
    last_succeeded_at = excluded.last_succeeded_at,
    last_attempted_at = excluded.last_attempted_at,
    next_run_at = excluded.next_run_at,
    last_error_category = excluded.last_error_category,
    safe_error_detail = excluded.safe_error_detail,
    confirmed_page = excluded.confirmed_page,
    confirmed_cursor = excluded.confirmed_cursor,
    priority = excluded.priority,
    generation = excluded.generation,
    consecutive_failures = excluded.consecutive_failures,
    consecutive_immediate_retries = excluded.consecutive_immediate_retries;
