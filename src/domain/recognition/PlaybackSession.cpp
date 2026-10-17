#include "PlaybackSession.h"

PlaybackSessionId::PlaybackSessionId(QString pluginId, QString sessionKey)
    : m_pluginId(std::move(pluginId)), m_sessionKey(std::move(sessionKey)) {}

std::optional<PlaybackSessionId> PlaybackSessionId::create(const QString &pluginId, const QString &sessionKey) {
    const auto trimmedPluginId = pluginId.trimmed();
    const auto trimmedSessionKey = sessionKey.trimmed();
    if (trimmedPluginId.isEmpty() || trimmedSessionKey.isEmpty()) return std::nullopt;
    return PlaybackSessionId(trimmedPluginId, trimmedSessionKey);
}

QString PlaybackSessionId::key() const {
    return m_pluginId + QLatin1Char(':') + m_sessionKey;
}
