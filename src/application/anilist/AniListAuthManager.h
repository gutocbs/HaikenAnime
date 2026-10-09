#ifndef HAIKENANIME_ANILISTAUTHMANAGER_H
#define HAIKENANIME_ANILISTAUTHMANAGER_H

#include "IAniListAuthProvider.h"
#include "ISecretStore.h"

#include "AniListOAuthCallbackParser.h"
#include "AniListOAuthConfig.h"
#include "IAniListViewerClient.h"

enum class AniListAuthenticationState { Disconnected, Authorizing, AwaitingValidation, Authenticated, AuthenticationFailed };

/** Loads and caches AniList credentials while keeping storage separate from API transport. */
class AniListAuthManager final : public IAniListAuthProvider {
public:
    /** Creates an authorization manager backed by the supplied secret store. */
    explicit AniListAuthManager(ISecretStore &secretStore);

    /** Loads credentials from storage and writes any failure description to error. */
    [[nodiscard]] bool load(QString &error);

    /** Saves credentials through the configured store and updates the in-memory cache. */
    [[nodiscard]] bool save(const AniListCredentials &credentials, QString &error);

    /** Removes persisted credentials and clears the in-memory cache. */
    [[nodiscard]] bool clear(QString &error);

    /** Starts the browser authorization flow and returns the AniList authorization URL. */
    [[nodiscard]] QUrl beginAuthorization(const AniListOAuthConfig &config);

    /** Parses an OAuth callback and caches its token until Viewer validation completes. */
    [[nodiscard]] bool handleCallback(const QUrl &callback, QString &error);

    /** Validates the cached OAuth token and persists the authenticated AniList identity. */
    [[nodiscard]] bool validateToken(IAniListViewerClient &viewerClient, QString &error);

    /** Returns the state of the AniList integration session. */
    [[nodiscard]] AniListAuthenticationState state() const;

    /** Returns the last successfully loaded or saved credentials. */
    [[nodiscard]] AniListCredentials credentials() const override;

private:
    ISecretStore &secretStore_;
    AniListCredentials credentials_;
    AniListAuthenticationState state_ = AniListAuthenticationState::Disconnected;
};

#endif // HAIKENANIME_ANILISTAUTHMANAGER_H
