#ifndef HAIKENANIME_SQLITEUSERPREFERENCESREPOSITORY_H
#define HAIKENANIME_SQLITEUSERPREFERENCESREPOSITORY_H

#include <QSqlDatabase>
#include "../../application/configuration/IUserPreferencesRepository.h"

class SqliteUserPreferencesRepository final : public IUserPreferencesRepository {
public:
    SqliteUserPreferencesRepository(QSqlDatabase database, QString readQuery, QString upsertQuery);
    [[nodiscard]] bool read(UserPreferences &preferences, bool &found, QString &error) override;
    [[nodiscard]] bool replace(const UserPreferences &preferences, QString &error) override;
private:
    QSqlDatabase database_;
    QString readQuery_;
    QString upsertQuery_;
};

#endif
