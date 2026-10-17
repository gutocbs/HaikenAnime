INSERT INTO plugin_setting (plugin_id, setting_key, setting_value)
VALUES (:plugin_id, :setting_key, :setting_value)
ON CONFLICT(plugin_id, setting_key) DO UPDATE SET
    setting_value = excluded.setting_value;
