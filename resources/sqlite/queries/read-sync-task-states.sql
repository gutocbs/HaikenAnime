SELECT task_kind, partition, cache_validity, last_succeeded_at, last_attempted_at,
       last_error_category, consecutive_failures, consecutive_immediate_retries
FROM sync_task_state
ORDER BY task_kind, partition;
