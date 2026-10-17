CREATE TABLE IF NOT EXISTS plugin_setting (
    plugin_id TEXT NOT NULL CHECK (length(trim(plugin_id)) > 0),
    setting_key TEXT NOT NULL CHECK (length(trim(setting_key)) > 0),
    setting_value TEXT NOT NULL,
    PRIMARY KEY (plugin_id, setting_key)
);
