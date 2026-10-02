UPDATE media
SET consumed_chapters = :consumed_chapters,
    personal_score = :personal_score,
    user_list_status = :user_list_status,
    local_path = :local_path,
    alternative_names = :alternative_names,
    source_removed_at = NULL
WHERE id = :id;
