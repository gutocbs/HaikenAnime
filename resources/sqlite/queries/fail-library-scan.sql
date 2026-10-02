UPDATE library_scans
SET status = :status, finished_at = :finished_at,
    observed_count = :observed_count, diagnostic = :diagnostic
WHERE id = :scan_id AND status = 'running';
