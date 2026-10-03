ALTER TABLE user_preferences ADD COLUMN include_adult_content INTEGER NOT NULL DEFAULT 0 CHECK (include_adult_content IN (0, 1));
