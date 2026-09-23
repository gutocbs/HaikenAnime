SELECT media_id, field, previous_value, new_value, created_at, local_updated_at,
       remote_observed_at, remote_version, attempts, status, last_error
FROM anilist_pending_changes
WHERE media_id = :media_id AND status IN (0, 1, 3, 4)
ORDER BY created_at ASC, id ASC
