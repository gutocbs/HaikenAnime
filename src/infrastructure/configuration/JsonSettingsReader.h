#ifndef HAIKENANIME_JSONSETTINGSREADER_H
#define HAIKENANIME_JSONSETTINGSREADER_H

#include <QString>

#include "../../application/configuration/Settings.h"

/** Reads AniList and HTTP settings from a JSON file. */
class JsonSettingsReader final {
public:
    /** Creates a reader for the supplied Settings.json path. */
    explicit JsonSettingsReader(QString filePath);

    /** Parses the settings file and reports missing or invalid values through error. */
    [[nodiscard]] bool read(Settings &settings, QString &error);

private:
    QString filePath_;
};

#endif // HAIKENANIME_JSONSETTINGSREADER_H
