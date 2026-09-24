#ifndef HAIKENANIME_ISETTINGSREADER_H
#define HAIKENANIME_ISETTINGSREADER_H

#include <QString>

#include "Settings.h"

class ISettingsReader {
public:
    virtual ~ISettingsReader() = default;

    /** Replaces settings with configured values and clears error on success. */
    [[nodiscard]] virtual bool read(Settings &settings, QString &error) = 0;
};

#endif // HAIKENANIME_ISETTINGSREADER_H
