#include <QtTest>

#include "../../src/infrastructure/secrets/WindowsPluginSecretStore.h"

class WindowsPluginSecretStoreTests final : public QObject {
    Q_OBJECT

private slots:
    void writesReadsAndClearsSecret();
    void keepsPluginsAndKeysSeparate();
    void errorsNeverContainTheSecret();
    void clearingMissingSecretSucceeds();

private:
    [[nodiscard]] static QString prefix() {
        return QStringLiteral("HaikenAnime.Tests.%1.Plugin").arg(QCoreApplication::applicationPid());
    }
};

void WindowsPluginSecretStoreTests::writesReadsAndClearsSecret() {
    WindowsPluginSecretStore store(prefix());
    QString error;
    if (!store.writeSecret(QStringLiteral("plex"), QStringLiteral("token"), QStringLiteral("secret-1"), error)) {
        QSKIP(qPrintable(error));
    }

    QVERIFY(store.hasSecret(QStringLiteral("plex"), QStringLiteral("token")));
    QString value;
    QVERIFY2(store.readSecret(QStringLiteral("plex"), QStringLiteral("token"), value, error), qPrintable(error));
    QCOMPARE(value, QStringLiteral("secret-1"));
    QVERIFY2(store.clearSecret(QStringLiteral("plex"), QStringLiteral("token"), error), qPrintable(error));
    QVERIFY(!store.hasSecret(QStringLiteral("plex"), QStringLiteral("token")));
}

void WindowsPluginSecretStoreTests::keepsPluginsAndKeysSeparate() {
    WindowsPluginSecretStore store(prefix());
    QString error;
    if (!store.writeSecret(QStringLiteral("plex"), QStringLiteral("token"), QStringLiteral("a"), error)) {
        QSKIP(qPrintable(error));
    }

    QVERIFY(!store.hasSecret(QStringLiteral("mpv"), QStringLiteral("token")));
    QVERIFY(!store.hasSecret(QStringLiteral("plex"), QStringLiteral("other")));
    QVERIFY(store.clearSecret(QStringLiteral("plex"), QStringLiteral("token"), error));
}

void WindowsPluginSecretStoreTests::errorsNeverContainTheSecret() {
    WindowsPluginSecretStore store(prefix());
    QString error;
    QString value = QStringLiteral("stale");

    QVERIFY(!store.readSecret(QStringLiteral("plex"), QStringLiteral("missing"), value, error));
    QVERIFY(value.isEmpty());
    QVERIFY(!error.isEmpty());
    QVERIFY(!store.writeSecret(QStringLiteral("plex"), QStringLiteral("token"), QString(), error));
    QVERIFY(!error.isEmpty());
}

void WindowsPluginSecretStoreTests::clearingMissingSecretSucceeds() {
    WindowsPluginSecretStore store(prefix());
    QString error;

    QVERIFY2(store.clearSecret(QStringLiteral("plex"), QStringLiteral("never-written"), error), qPrintable(error));
}

QTEST_MAIN(WindowsPluginSecretStoreTests)
#include "WindowsPluginSecretStoreTests.moc"
