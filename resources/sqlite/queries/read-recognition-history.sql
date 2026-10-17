SELECT id, plugin_id, session_key, user_id, device_id, deduplication_key, title, media_id, season, episode, confidence,
       correction_title, correction_media_id, correction_season, correction_episode, correction_confidence,
       status, diagnostic, sync_state, sync_error, created_at, updated_at
FROM recognition_record
WHERE (:record_id IS NULL OR id = :record_id)
  AND (:deduplication_key IS NULL OR deduplication_key = :deduplication_key)
  AND (:status IS NULL OR status = :status)
  AND (:plugin_id IS NULL OR plugin_id = :plugin_id)
  -- Effective episode: the correction when present, otherwise the recognized candidate.
  AND (:media_id IS NULL
       OR (CASE WHEN correction_episode IS NOT NULL THEN correction_media_id ELSE media_id END) = :media_id)
  AND (:max_episode IS NULL
       OR (CASE WHEN correction_episode IS NOT NULL THEN correction_episode ELSE episode END) <= :max_episode)
  AND (:awaiting_sync = 0 OR sync_state IN ('pending', 'failed'))
  AND (:before_id IS NULL OR id < :before_id)
ORDER BY id DESC
LIMIT :limit;
