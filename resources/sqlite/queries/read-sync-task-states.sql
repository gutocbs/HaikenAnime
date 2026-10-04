SELECT task_kind, partition, status, cache_validity, last_succeeded_at, last_attempted_at,
       next_run_at, last_error_category, safe_error_detail, confirmed_page, confirmed_cursor,
       priority, generation, consecutive_failures, consecutive_immediate_retries
FROM sync_task_state
ORDER BY task_kind, partition;
