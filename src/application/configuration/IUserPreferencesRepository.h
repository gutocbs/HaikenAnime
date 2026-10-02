#ifndef HAIKENANIME_IUSERPREFERENCESREPOSITORY_H
#define HAIKENANIME_IUSERPREFERENCESREPOSITORY_H

#include <QString>
#include "UserPreferences.h"

class IUserPreferencesRepository {
public:
    virtual ~IUserPreferencesRepository() = default;
    [[nodiscard]] virtual bool read(UserPreferences &preferences, bool &found, QString &error) = 0;
    [[nodiscard]] virtual bool replace(const UserPreferences &preferences, QString &error) = 0;
};

#endif
