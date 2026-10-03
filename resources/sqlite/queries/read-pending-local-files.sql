SELECT id, root_path, relative_path, normalized_relative_path, file_name, size_bytes,
       modified_at, available, recognition_state, extracted_title, media_kind,
       season, episode, media_id, recognition_diagnostic
FROM local_files
WHERE root_path = :root_path AND available = 1
  AND recognition_state IN ('unprocessed', 'unrecognized', 'ambiguous')
ORDER BY CASE recognition_state WHEN 'unprocessed' THEN 0 ELSE 1 END,
         normalized_relative_path COLLATE NOCASE ASC;
