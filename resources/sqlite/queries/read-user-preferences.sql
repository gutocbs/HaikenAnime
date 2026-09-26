SELECT score_minimum,
       score_maximum,
       score_step,
       cover_quality,
       synchronization_enabled,
       synchronization_interval_ms
FROM user_preferences
WHERE id = 1
