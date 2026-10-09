#include <QtTest>

#include <QUrlQuery>

#include "../../src/application/anilist/AniListOAuthCallbackParser.h"
#include "../../src/application/anilist/AniListOAuthConfig.h"

class AniListOAuthCallbackParserTests : public QObject {
    Q_OBJECT

private slots:
    void createsImplicitGrantUrlWithoutRedirectUri();
    void extractsAccessTokenFromCallbackFragment();
    void rejectsTokenInQueryInsteadOfFragment();
    void rejectsCallbackWithoutAccessToken();
    void reportsCallbackErrorWithoutLeakingToken();
};

void AniListOAuthCallbackParserTests::createsImplicitGrantUrlWithoutRedirectUri() {
    AniListOAuthConfig config(QStringLiteral("12345"), QUrl(QStringLiteral("haikenanime://oauth/callback")));

    const QUrl url = config.authorizationUrl();

    QCOMPARE(url.scheme(), QStringLiteral("https"));
    QCOMPARE(url.host(), QStringLiteral("anilist.co"));
    QCOMPARE(url.path(), QStringLiteral("/api/v2/oauth/authorize"));
    const QUrlQuery query(url);
    QCOMPARE(query.queryItemValue(QStringLiteral("client_id")), QStringLiteral("12345"));
    QVERIFY(!query.hasQueryItem(QStringLiteral("redirect_uri")));
    QCOMPARE(query.queryItemValue(QStringLiteral("response_type")), QStringLiteral("token"));
}

void AniListOAuthCallbackParserTests::extractsAccessTokenFromCallbackFragment() {
    AniListOAuthCallbackParser parser;
    AniListCredentials credentials;
    QString error;

    QVERIFY(parser.parse(QUrl(QStringLiteral("haikenanime://oauth/callback#access_token=test-token&token_type=Bearer")), credentials, error));
    QCOMPARE(credentials.token, QStringLiteral("test-token"));
}

void AniListOAuthCallbackParserTests::rejectsTokenInQueryInsteadOfFragment() {
    AniListOAuthCallbackParser parser;
    AniListCredentials credentials;
    QString error;

    QVERIFY(!parser.parse(QUrl(QStringLiteral("haikenanime://oauth/callback?access_token=test-token")), credentials, error));
    QCOMPARE(error, QStringLiteral("AniList OAuth callback does not contain an access token fragment."));
}

void AniListOAuthCallbackParserTests::rejectsCallbackWithoutAccessToken() {
    AniListOAuthCallbackParser parser;
    AniListCredentials credentials;
    QString error;

    QVERIFY(!parser.parse(QUrl(QStringLiteral("haikenanime://oauth/callback#token_type=Bearer")), credentials, error));
    QCOMPARE(error, QStringLiteral("AniList OAuth callback does not contain an access token fragment."));
}

void AniListOAuthCallbackParserTests::reportsCallbackErrorWithoutLeakingToken() {
    AniListOAuthCallbackParser parser;
    AniListCredentials credentials;
    QString error;

    QVERIFY(!parser.parse(QUrl(QStringLiteral("haikenanime://oauth/callback#error=access_denied&error_description=No+thanks")), credentials, error));
    QCOMPARE(error, QStringLiteral("AniList authorization was denied: No thanks"));
}

QTEST_MAIN(AniListOAuthCallbackParserTests)
#include "AniListOAuthCallbackParserTests.moc"
