SELECT id, name, english_name, original_name, alternative_names,
       total_chapters, consumed_chapters, next_chapter, average_score, personal_score, local_path,
       cover_url, cover_medium_url, cover_large_url, cover_extra_large_url,
       synopsis, type, status, user_list_status,
       season, season_year, next_airing_episode, next_airing_at, anilist_url, external_links
FROM media
WHERE source_removed_at IS NULL
ORDER BY status ASC, name COLLATE NOCASE ASC;
