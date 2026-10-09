ALTER TABLE user_preferences ADD COLUMN enabled_user_lists TEXT NOT NULL DEFAULT '["current","planning","on_hold","dropped","completed"]';
