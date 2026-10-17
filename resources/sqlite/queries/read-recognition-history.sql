SELECT id, plugin_id, session_key, user_id, device_id, deduplication_key, title, media_id, season, episode, confidence,
       correction_title, correction_media_id, correction_season, correction_episode, correction_confidence,
       status, diagnostic, sync_state, sync_error, created_at, updated_at
FROM recognition_record
WHERE (:record_id IS NULL OR id = :record_id)
  AND (:deduplication_key IS NULL OR deduplication_key = :deduplication_key)
  AND (:status IS NULL OR status = :status)
  AND (:plugin_id IS NULL OR plugin_id = :plugin_id)
  AND (:before_id IS NULL OR id < :before_id)
ORDER BY id DESC
LIMIT :limit;
