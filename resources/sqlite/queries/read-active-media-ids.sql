SELECT id
FROM media
WHERE source_removed_at IS NULL
ORDER BY id;
