INSERT INTO user_preferences (
    id, score_minimum, score_maximum, score_step, cover_quality,
    synchronization_enabled, synchronization_interval_ms
) VALUES (
    1, :score_minimum, :score_maximum, :score_step, :cover_quality,
    :synchronization_enabled, :synchronization_interval_ms
)
ON CONFLICT(id) DO UPDATE SET
    score_minimum = excluded.score_minimum,
    score_maximum = excluded.score_maximum,
    score_step = excluded.score_step,
    cover_quality = excluded.cover_quality,
    synchronization_enabled = excluded.synchronization_enabled,
    synchronization_interval_ms = excluded.synchronization_interval_ms
