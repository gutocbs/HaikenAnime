UPDATE media
SET source_removed_at = :source_removed_at
WHERE id = :id
  AND source_removed_at IS NULL;
