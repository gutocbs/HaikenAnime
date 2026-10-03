#ifndef HAIKENANIME_USERPREFERENCES_H
#define HAIKENANIME_USERPREFERENCES_H

#include <QStringList>

#include "../covers/CoverQuality.h"
#include "CardStatusPresentation.h"
#include "LanguagePreference.h"
#include "../media/MediaTitleResolver.h"

struct UserPreferences final {
    double scoreMinimum = 0.0;
    double scoreMaximum = 10.0;
    double scoreStep = 1.0;
    CoverQuality coverQuality = CoverQuality::Medium;
    bool synchronizationEnabled = true;
    int synchronizationIntervalMs = 3600000;
    QString homeSortKey = QStringLiteral("title_asc");
    CardStatusPresentation cardStatusPresentation = CardStatusPresentation::PersonalListStatus;
    QString languageKey = DefaultLanguageKey();
    QString preferredTitleKey = DefaultPreferredTitleKey();
    bool includeAdultContent = false;
    QString libraryRoot = QStringLiteral("Q:\\");
    QStringList scanExtensions = {QStringLiteral(".mkv"), QStringLiteral(".mp4"),
                                  QStringLiteral(".avi"), QStringLiteral(".webm"),
                                  QStringLiteral(".m4v"), QStringLiteral(".mov"),
                                  QStringLiteral(".wmv"), QStringLiteral(".ts")};
    bool automaticLocalFileRecognition = true;
};

inline bool operator==(const UserPreferences &left, const UserPreferences &right) {
    return left.scoreMinimum == right.scoreMinimum
        && left.scoreMaximum == right.scoreMaximum
        && left.scoreStep == right.scoreStep
        && left.coverQuality == right.coverQuality
        && left.synchronizationEnabled == right.synchronizationEnabled
        && left.synchronizationIntervalMs == right.synchronizationIntervalMs
        && left.homeSortKey == right.homeSortKey
        && left.cardStatusPresentation == right.cardStatusPresentation
        && left.languageKey == right.languageKey
        && left.preferredTitleKey == right.preferredTitleKey
        && left.includeAdultContent == right.includeAdultContent
        && left.libraryRoot == right.libraryRoot
        && left.scanExtensions == right.scanExtensions
        && left.automaticLocalFileRecognition == right.automaticLocalFileRecognition;
}

Q_DECLARE_METATYPE(UserPreferences)

#endif
