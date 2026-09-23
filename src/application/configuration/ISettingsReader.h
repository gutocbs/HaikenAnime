#ifndef HAIKENANIME_ISETTINGSREADER_H
#define HAIKENANIME_ISETTINGSREADER_H

#include <QString>

#include "Settings.h"

class ISettingsReader {
public:
    virtual ~ISettingsReader() = default;

    /** Reads application settings from the configured source and reports errors through error. */
    [[nodiscard]] virtual bool read(Settings &settings, QString &error) = 0;
};

#endif // HAIKENANIME_ISETTINGSREADER_H
