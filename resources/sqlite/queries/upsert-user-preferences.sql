INSERT INTO user_preferences (
    id, score_minimum, score_maximum, score_step, cover_quality,
    synchronization_enabled, synchronization_interval_ms, library_root, scan_extensions, home_sort_key,
    card_status_presentation, language_key
) VALUES (
    1, :score_minimum, :score_maximum, :score_step, :cover_quality,
    :synchronization_enabled, :synchronization_interval_ms, :library_root, :scan_extensions, :home_sort_key,
    :card_status_presentation, :language_key
)
ON CONFLICT(id) DO UPDATE SET
    score_minimum = excluded.score_minimum,
    score_maximum = excluded.score_maximum,
    score_step = excluded.score_step,
    cover_quality = excluded.cover_quality,
    synchronization_enabled = excluded.synchronization_enabled,
    synchronization_interval_ms = excluded.synchronization_interval_ms,
    library_root = excluded.library_root,
    scan_extensions = excluded.scan_extensions,
    home_sort_key = excluded.home_sort_key,
    card_status_presentation = excluded.card_status_presentation,
    language_key = excluded.language_key
