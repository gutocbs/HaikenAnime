UPDATE local_files
SET available = 0
WHERE root_path = (SELECT root_path FROM library_scans WHERE id = :scan_id AND status = 'running')
  AND last_seen_scan_id <> :scan_id
  AND available = 1;
