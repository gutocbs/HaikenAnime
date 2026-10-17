INSERT INTO recognition_record
    (plugin_id, session_key, user_id, device_id, deduplication_key, title, media_id, season, episode, confidence,
     correction_title, correction_media_id, correction_season, correction_episode, correction_confidence,
     status, diagnostic, sync_state, sync_error, created_at, updated_at)
VALUES
    (:plugin_id, :session_key, :user_id, :device_id, :deduplication_key, :title, :media_id, :season, :episode,
     :confidence, :correction_title, :correction_media_id, :correction_season, :correction_episode,
     :correction_confidence, :status, :diagnostic, :sync_state, :sync_error, :created_at, :updated_at);
