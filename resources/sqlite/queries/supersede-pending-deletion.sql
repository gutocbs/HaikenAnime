UPDATE anilist_pending_changes
SET status = 5,
    local_updated_at = :local_updated_at
WHERE media_id = :media_id
  AND field = 9
  AND status IN (0, 1, 3, 4);
