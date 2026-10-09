#include <QtTest>

#include "../../src/infrastructure/anilist/WindowsAniListOAuthLauncher.h"

class WindowsAniListOAuthLauncherTests : public QObject {
    Q_OBJECT

private slots:
    void opensAuthorizationUrl();
    void reportsFailureToOpenAuthorizationUrl();
};

void WindowsAniListOAuthLauncherTests::opensAuthorizationUrl() {
    QUrl opened;
    WindowsAniListOAuthLauncher launcher([&opened](const QUrl &url) { opened = url; return true; });
    QString error;
    const QUrl authorizationUrl(QStringLiteral("https://anilist.co/api/v2/oauth/authorize?client_id=123"));

    QVERIFY2(launcher.launch(authorizationUrl, error), qPrintable(error));
    QCOMPARE(opened, authorizationUrl);
}

void WindowsAniListOAuthLauncherTests::reportsFailureToOpenAuthorizationUrl() {
    WindowsAniListOAuthLauncher launcher([](const QUrl &) { return false; });
    QString error;

    QVERIFY(!launcher.launch(QUrl(QStringLiteral("https://anilist.co")), error));
    QCOMPARE(error, QStringLiteral("Could not open AniList authorization URL."));
}

QTEST_MAIN(WindowsAniListOAuthLauncherTests)
#include "WindowsAniListOAuthLauncherTests.moc"
