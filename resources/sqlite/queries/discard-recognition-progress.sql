UPDATE anilist_pending_changes
SET status = :superseded_status
WHERE media_id = :media_id
  AND field = :progress_field
  AND status IN (:pending_status, :failed_status, :confirmation_status)
  AND json_extract(new_value, '$.type') = 'int'
  AND json_extract(new_value, '$.value') = :episode;
