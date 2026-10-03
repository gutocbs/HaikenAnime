SELECT media_id, episode,
       rtrim(root_path, '/\\') || '/' || ltrim(relative_path, '/\\') AS path
FROM local_files
WHERE media_id = :media_id
  AND available = 1
  AND media_kind = 'anime'
  AND recognition_state = 'associated'
  AND typeof(episode) = 'integer'
  AND episode > :consumed_episode
ORDER BY episode ASC, normalized_relative_path COLLATE NOCASE ASC
LIMIT 1;
