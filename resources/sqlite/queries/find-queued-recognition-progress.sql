SELECT 1
FROM anilist_pending_changes
WHERE media_id = :media_id
  AND field = :progress_field
  AND status <> :superseded_status
  AND json_extract(new_value, '$.type') = 'int'
  AND json_extract(new_value, '$.value') >= :episode
LIMIT 1;
