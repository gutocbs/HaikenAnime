UPDATE anilist_pending_changes
SET status = :status, attempts = :attempts, last_error = :last_error
WHERE media_id = :media_id AND field = :field AND status IN (0, 1, 3, 4)
