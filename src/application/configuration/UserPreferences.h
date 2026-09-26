#ifndef HAIKENANIME_USERPREFERENCES_H
#define HAIKENANIME_USERPREFERENCES_H

#include <QStringList>

#include "../covers/CoverQuality.h"

struct UserPreferences final {
    double scoreMinimum = 0.0;
    double scoreMaximum = 10.0;
    double scoreStep = 1.0;
    CoverQuality coverQuality = CoverQuality::Medium;
    bool synchronizationEnabled = true;
    int synchronizationIntervalMs = 3600000;
    QString libraryRoot = QStringLiteral("Q:\\");
    QStringList scanExtensions = {QStringLiteral(".mkv"), QStringLiteral(".mp4"),
                                  QStringLiteral(".avi"), QStringLiteral(".webm"),
                                  QStringLiteral(".m4v"), QStringLiteral(".mov"),
                                  QStringLiteral(".wmv"), QStringLiteral(".ts")};
};

inline bool operator==(const UserPreferences &left, const UserPreferences &right) {
    return left.scoreMinimum == right.scoreMinimum
        && left.scoreMaximum == right.scoreMaximum
        && left.scoreStep == right.scoreStep
        && left.coverQuality == right.coverQuality
        && left.synchronizationEnabled == right.synchronizationEnabled
        && left.synchronizationIntervalMs == right.synchronizationIntervalMs
        && left.libraryRoot == right.libraryRoot
        && left.scanExtensions == right.scanExtensions;
}

Q_DECLARE_METATYPE(UserPreferences)

#endif
