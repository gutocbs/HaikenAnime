INSERT INTO media (id, name, english_name, original_name, alternative_names,
                   total_chapters, average_score, cover_url, synopsis, type, status, user_list_status)
VALUES (:id, :name, :english_name, :original_name, :alternative_names,
        :total_chapters, :average_score, :cover_url, :synopsis, :type, :status, :user_list_status)
ON CONFLICT(id) DO UPDATE SET
    name = excluded.name,
    english_name = excluded.english_name,
    original_name = excluded.original_name,
    alternative_names = excluded.alternative_names,
    total_chapters = excluded.total_chapters,
    average_score = excluded.average_score,
    cover_url = excluded.cover_url,
    synopsis = excluded.synopsis,
    type = excluded.type,
    status = excluded.status,
    user_list_status = CASE WHEN excluded.user_list_status = -1
                            THEN media.user_list_status ELSE excluded.user_list_status END,
    source_removed_at = NULL
