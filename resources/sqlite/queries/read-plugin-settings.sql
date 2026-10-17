SELECT setting_key, setting_value
FROM plugin_setting
WHERE plugin_id = :plugin_id
ORDER BY setting_key;
