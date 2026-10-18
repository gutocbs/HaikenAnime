#ifndef HAIKENANIME_SQLITEPLUGINSETTINGSREPOSITORY_H
#define HAIKENANIME_SQLITEPLUGINSETTINGSREPOSITORY_H

#include <QSqlDatabase>

#include "../../application/recognition/IPluginSettingsRepository.h"

/// Stores each plugin setting as one JSON-encoded row in plugin_setting.
class SqlitePluginSettingsRepository final : public IPluginSettingsRepository {
public:
    explicit SqlitePluginSettingsRepository(QSqlDatabase database);

    bool read(const QString &pluginId, QVariantMap &values, QString &error) override;
    bool replace(const QString &pluginId, const QVariantMap &values, QString &error) override;

private:
    QSqlDatabase database_;
};

#endif // HAIKENANIME_SQLITEPLUGINSETTINGSREPOSITORY_H
