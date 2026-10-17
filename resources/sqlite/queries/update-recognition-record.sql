UPDATE recognition_record SET
    title = :title,
    media_id = :media_id,
    season = :season,
    episode = :episode,
    confidence = :confidence,
    correction_title = :correction_title,
    correction_media_id = :correction_media_id,
    correction_season = :correction_season,
    correction_episode = :correction_episode,
    correction_confidence = :correction_confidence,
    status = :status,
    diagnostic = :diagnostic,
    sync_state = :sync_state,
    sync_error = :sync_error,
    updated_at = :updated_at
WHERE id = :id;
