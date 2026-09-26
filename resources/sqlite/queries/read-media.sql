SELECT id, name, english_name, original_name, alternative_names,
       total_chapters, consumed_chapters, next_chapter, average_score, personal_score,
       cover_url, synopsis, type, status, user_list_status
FROM media
WHERE source_removed_at IS NULL
ORDER BY status ASC, name COLLATE NOCASE ASC;
