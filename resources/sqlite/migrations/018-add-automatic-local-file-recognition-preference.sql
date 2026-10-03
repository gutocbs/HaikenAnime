ALTER TABLE user_preferences ADD COLUMN automatic_local_file_recognition INTEGER NOT NULL DEFAULT 1 CHECK (automatic_local_file_recognition IN (0, 1));
