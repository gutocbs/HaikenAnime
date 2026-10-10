SELECT media_id
FROM anilist_pending_changes
WHERE status IN (0, 1, 3)
GROUP BY media_id
ORDER BY MIN(created_at) ASC, media_id ASC
