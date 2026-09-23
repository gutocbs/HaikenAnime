#ifndef HAIKENANIME_ANILISTGRAPHQLCLIENT_H
#define HAIKENANIME_ANILISTGRAPHQLCLIENT_H

#include <QJsonObject>
#include <QString>
#include <QUrl>

#include "../../application/anilist/IAniListAuthProvider.h"
#include "../../domain/anilist/AniListGraphQlResponse.h"

class QNetworkAccessManager;

/** Sends read-only GraphQL operations to AniList through a supplied network manager. */
class AniListGraphQlClient final {
public:
    /** Creates a client using the supplied manager and optional authorization provider. */
    explicit AniListGraphQlClient(QNetworkAccessManager &networkManager,
                                  IAniListAuthProvider *authProvider = nullptr,
                                  QUrl endpoint = QUrl(QStringLiteral("https://graphql.anilist.co")),
                                  int timeoutMs = 30000, int maxRetries = 0,
                                  int retryDelayMs = 0);

    /**
     * Sends a GraphQL POST request and parses both data and GraphQL errors.
     * The call waits for the network reply and must therefore run outside the UI thread.
     */
    [[nodiscard]] bool execute(const QString &query, const QJsonObject &variables,
                               AniListGraphQlResponse &response, QString &error) const;

    /** Returns the endpoint currently used by the client. */
    [[nodiscard]] QUrl endpoint() const;

private:
    [[nodiscard]] bool ExecuteWithAttempt(const QString &query, const QJsonObject &variables,
                                          AniListGraphQlResponse &response, QString &error,
                                          int attempt) const;

    QNetworkAccessManager &networkManager_;
    IAniListAuthProvider *authProvider_;
    QUrl endpoint_;
    int timeoutMs_;
    int maxRetries_;
    int retryDelayMs_;
};

#endif // HAIKENANIME_ANILISTGRAPHQLCLIENT_H
