#ifndef HAIKENANIME_WINDOWSANILISTOAUTHLAUNCHER_H
#define HAIKENANIME_WINDOWSANILISTOAUTHLAUNCHER_H

#include <QUrl>

#include <functional>

/** Opens the AniList authorization URL through the desktop shell. */
class WindowsAniListOAuthLauncher final {
public:
    using OpenUrl = std::function<bool(const QUrl &)>;

    explicit WindowsAniListOAuthLauncher(OpenUrl openUrl = {});

    /** Opens the supplied authorization URL and reports a user-safe failure. */
    [[nodiscard]] bool launch(const QUrl &authorizationUrl, QString &error) const;

private:
    OpenUrl openUrl_;
};

#endif // HAIKENANIME_WINDOWSANILISTOAUTHLAUNCHER_H
