SELECT media_id
FROM anilist_pending_changes
WHERE owner_user_id = :owner AND status IN (0, 1, 3)
GROUP BY media_id
ORDER BY MIN(created_at) ASC, media_id ASC
