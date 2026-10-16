UPDATE anilist_pending_changes
SET owner_user_id = :owner
WHERE owner_user_id = 0
