#ifndef HAIKENANIME_SQLITEUSERPREFERENCESREPOSITORY_H
#define HAIKENANIME_SQLITEUSERPREFERENCESREPOSITORY_H

#include <QSqlDatabase>
#include "../../application/configuration/IUserPreferencesRepository.h"

class SqliteUserPreferencesRepository final : public IUserPreferencesRepository {
public:
    explicit SqliteUserPreferencesRepository(QSqlDatabase database);
    [[nodiscard]] bool read(UserPreferences &preferences, bool &found, QString &error) override;
    [[nodiscard]] bool read(UserPreferences &preferences, bool &found, QString &error,
                            QString &warning);
    [[nodiscard]] bool replace(const UserPreferences &preferences, QString &error) override;
private:
    QSqlDatabase database_;
};

#endif
