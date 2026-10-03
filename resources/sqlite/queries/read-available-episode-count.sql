SELECT COUNT(*)
FROM local_files
WHERE media_id = :media_id
  AND available = 1
  AND media_kind = 'anime'
  AND recognition_state = 'associated'
  AND typeof(episode) = 'integer';
