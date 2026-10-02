INSERT INTO cover_cache
    (media_id, remote_url, quality, relative_path, mime_type, byte_size,
     etag, last_modified, validated_at)
VALUES
    (:media_id, :remote_url, :quality, :relative_path, :mime_type, :byte_size,
     :etag, :last_modified, :validated_at)
ON CONFLICT(media_id) DO UPDATE SET
    remote_url = excluded.remote_url,
    quality = excluded.quality,
    relative_path = excluded.relative_path,
    mime_type = excluded.mime_type,
    byte_size = excluded.byte_size,
    etag = excluded.etag,
    last_modified = excluded.last_modified,
    validated_at = excluded.validated_at
