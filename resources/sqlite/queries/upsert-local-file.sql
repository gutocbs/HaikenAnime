INSERT INTO local_files (
    root_path, relative_path, normalized_relative_path, file_name, extension,
    size_bytes, modified_at, available, last_seen_scan_id
)
SELECT root_path, :relative_path, :normalized_relative_path, :file_name, :extension,
       :size_bytes, :modified_at, 1, id
FROM library_scans
WHERE id = :scan_id AND root_path = :root_path AND status = 'running'
ON CONFLICT(root_path, normalized_relative_path) DO UPDATE SET
    relative_path = excluded.relative_path,
    file_name = excluded.file_name,
    extension = excluded.extension,
    size_bytes = excluded.size_bytes,
    modified_at = excluded.modified_at,
    available = 1,
    last_seen_scan_id = excluded.last_seen_scan_id,
    recognition_state = CASE WHEN local_files.file_name <> excluded.file_name
                                   OR local_files.size_bytes <> excluded.size_bytes
                                   OR local_files.modified_at <> excluded.modified_at
                              THEN 'unprocessed' ELSE local_files.recognition_state END,
    extracted_title = CASE WHEN local_files.file_name <> excluded.file_name
                                OR local_files.size_bytes <> excluded.size_bytes
                                OR local_files.modified_at <> excluded.modified_at
                           THEN '' ELSE local_files.extracted_title END,
    media_kind = CASE WHEN local_files.file_name <> excluded.file_name
                           OR local_files.size_bytes <> excluded.size_bytes
                           OR local_files.modified_at <> excluded.modified_at
                      THEN 'anime' ELSE local_files.media_kind END,
    season = CASE WHEN local_files.file_name <> excluded.file_name
                       OR local_files.size_bytes <> excluded.size_bytes
                       OR local_files.modified_at <> excluded.modified_at
                  THEN NULL ELSE local_files.season END,
    episode = CASE WHEN local_files.file_name <> excluded.file_name
                        OR local_files.size_bytes <> excluded.size_bytes
                        OR local_files.modified_at <> excluded.modified_at
                   THEN NULL ELSE local_files.episode END,
    media_id = CASE WHEN local_files.file_name <> excluded.file_name
                         OR local_files.size_bytes <> excluded.size_bytes
                         OR local_files.modified_at <> excluded.modified_at
                    THEN NULL ELSE local_files.media_id END,
    recognition_diagnostic = CASE WHEN local_files.file_name <> excluded.file_name
                                       OR local_files.size_bytes <> excluded.size_bytes
                                       OR local_files.modified_at <> excluded.modified_at
                                  THEN '' ELSE local_files.recognition_diagnostic END,
    recognized_at = CASE WHEN local_files.file_name <> excluded.file_name
                              OR local_files.size_bytes <> excluded.size_bytes
                              OR local_files.modified_at <> excluded.modified_at
                         THEN NULL ELSE local_files.recognized_at END;
