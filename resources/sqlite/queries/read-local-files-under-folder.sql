SELECT id, root_path, relative_path, normalized_relative_path, file_name, size_bytes,
       modified_at, available, recognition_state, extracted_title, media_kind,
       season, episode, media_id, recognition_diagnostic
FROM local_files
WHERE available = 1
  AND substr(rtrim(replace(root_path, '\', '/'), '/') || '/' || ltrim(replace(relative_path, '\', '/'), '/'),
             1, length(:folder_prefix)) = :folder_prefix COLLATE NOCASE
ORDER BY normalized_relative_path COLLATE NOCASE ASC;
