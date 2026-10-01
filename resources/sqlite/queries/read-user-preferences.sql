SELECT score_minimum,
       score_maximum,
       score_step,
       cover_quality,
       synchronization_enabled,
       synchronization_interval_ms,
       library_root,
       scan_extensions,
       home_sort_key,
       card_status_presentation,
       language_key
FROM user_preferences
WHERE id = 1
