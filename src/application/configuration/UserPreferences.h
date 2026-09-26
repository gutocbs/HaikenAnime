#ifndef HAIKENANIME_USERPREFERENCES_H
#define HAIKENANIME_USERPREFERENCES_H

#include "../covers/CoverQuality.h"

struct UserPreferences final {
    double scoreMinimum = 0.0;
    double scoreMaximum = 10.0;
    double scoreStep = 1.0;
    CoverQuality coverQuality = CoverQuality::Medium;
    bool synchronizationEnabled = true;
    int synchronizationIntervalMs = 3600000;
};

inline bool operator==(const UserPreferences &left, const UserPreferences &right) {
    return left.scoreMinimum == right.scoreMinimum
        && left.scoreMaximum == right.scoreMaximum
        && left.scoreStep == right.scoreStep
        && left.coverQuality == right.coverQuality
        && left.synchronizationEnabled == right.synchronizationEnabled
        && left.synchronizationIntervalMs == right.synchronizationIntervalMs;
}

Q_DECLARE_METATYPE(UserPreferences)

#endif
