INSERT INTO anilist_pending_changes
(media_id, field, previous_value, new_value, created_at, local_updated_at,
 remote_observed_at, remote_version, attempts, status, last_error, owner_user_id)
VALUES (:media_id, :field, :previous_value, :new_value, :created_at, :local_updated_at,
        :remote_observed_at, :remote_version, :attempts, :status, :last_error, :owner_user_id)
