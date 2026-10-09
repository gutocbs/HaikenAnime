#include "WindowsAniListOAuthLauncher.h"

#include <QDesktopServices>

#include <utility>

WindowsAniListOAuthLauncher::WindowsAniListOAuthLauncher(OpenUrl openUrl)
    : openUrl_(std::move(openUrl)) {
}

bool WindowsAniListOAuthLauncher::launch(const QUrl &authorizationUrl, QString &error) const {
    error.clear();
    const bool opened = openUrl_ ? openUrl_(authorizationUrl)
                                 : QDesktopServices::openUrl(authorizationUrl);
    if (!opened) {
        error = QStringLiteral("Could not open AniList authorization URL.");
        return false;
    }
    return true;
}
