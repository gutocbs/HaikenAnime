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
    last_seen_scan_id = excluded.last_seen_scan_id;
