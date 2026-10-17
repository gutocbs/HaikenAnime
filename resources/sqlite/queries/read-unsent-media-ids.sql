SELECT DISTINCT media_id
FROM anilist_pending_changes
WHERE status IN (0, 1, 3, 4)
ORDER BY media_id ASC
