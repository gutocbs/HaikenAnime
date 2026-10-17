SELECT id, root_path, relative_path, normalized_relative_path, file_name, size_bytes,
       modified_at, available, recognition_state, extracted_title, media_kind,
       season, episode, media_id, recognition_diagnostic
FROM local_files
WHERE available = 1
  AND root_path = :root_path
  AND (:prefix = ''
       OR substr(normalized_relative_path, 1, length(:prefix)) = :prefix)
ORDER BY normalized_relative_path ASC;
