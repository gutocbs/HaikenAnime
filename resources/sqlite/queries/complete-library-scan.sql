UPDATE library_scans
SET status = 'succeeded', finished_at = :finished_at, observed_count = :observed_count
WHERE id = :scan_id AND status = 'running';
