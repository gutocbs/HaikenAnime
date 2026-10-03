UPDATE local_files
SET recognition_state = :recognition_state,
    extracted_title = :extracted_title,
    media_kind = :media_kind,
    season = :season,
    episode = :episode,
    media_id = :media_id,
    recognition_diagnostic = COALESCE(:diagnostic, ''),
    recognized_at = :recognized_at
WHERE id = :id;
