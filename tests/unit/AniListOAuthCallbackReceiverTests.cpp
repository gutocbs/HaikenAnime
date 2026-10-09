#include <QElapsedTimer>
#include <QEventLoop>
#include <QSignalSpy>
#include <QLocalSocket>
#include <QtTest>

#include <thread>

#include "../../src/app/AniListOAuthCallbackReceiver.h"

class AniListOAuthCallbackReceiverTests final : public QObject {
    Q_OBJECT

private slots:
    void findsCallbackUriInApplicationArguments();
    void deliversCallbackFromLocalClient();
    void forwardsCallbackToExistingServer();
};

void AniListOAuthCallbackReceiverTests::findsCallbackUriInApplicationArguments() {
    const auto callback = AniListOAuthCallbackReceiver::callbackFromArguments(
        {QStringLiteral("HaikenAnime.exe"),
         QStringLiteral("haikenanime://oauth/callback#access_token=opaque-token")});

    QVERIFY(callback.has_value());
    QCOMPARE(callback->scheme(), QStringLiteral("haikenanime"));
    QCOMPARE(callback->host(), QStringLiteral("oauth"));
}

void AniListOAuthCallbackReceiverTests::deliversCallbackFromLocalClient() {
    const QString serverName = QStringLiteral("HaikenAnimeOAuthTest.%1")
        .arg(QCoreApplication::applicationPid());
    AniListOAuthCallbackReceiver owner(serverName);
    QString error;
    QCOMPARE(owner.start({}, error), AniListOAuthCallbackReceiver::StartResult::Listening);
    QVERIFY(error.isEmpty());
    QSignalSpy received(&owner, &AniListOAuthCallbackReceiver::callbackReceived);

    const QUrl callback(QStringLiteral("haikenanime://oauth/callback#access_token=opaque-token"));
    QLocalSocket client;
    client.connectToServer(serverName);
    QVERIFY(client.waitForConnected(1000));
    QCOMPARE(client.write(callback.toString(QUrl::FullyEncoded).toUtf8()),
             qint64(callback.toString(QUrl::FullyEncoded).toUtf8().size()));

    QTRY_COMPARE(received.count(), 1);
    QCOMPARE(received.first().first().toUrl(), callback);
}

void AniListOAuthCallbackReceiverTests::forwardsCallbackToExistingServer() {
    const QString serverName = QStringLiteral("HaikenAnimeOAuthForwardingTest.%1")
        .arg(QCoreApplication::applicationPid());
    AniListOAuthCallbackReceiver owner(serverName);
    QString error;
    QCOMPARE(owner.start({}, error), AniListOAuthCallbackReceiver::StartResult::Listening);
    QSignalSpy received(&owner, &AniListOAuthCallbackReceiver::callbackReceived);

    const QUrl callback(QStringLiteral("haikenanime://oauth/callback#access_token=opaque-token"));
    AniListOAuthCallbackReceiver::StartResult result = AniListOAuthCallbackReceiver::StartResult::Unavailable;
    QString forwardingError;
    std::thread callbackProcess([&] {
        AniListOAuthCallbackReceiver receiver(serverName);
        result = receiver.start({QStringLiteral("HaikenAnime.exe"), callback.toString(QUrl::FullyEncoded)},
                                forwardingError);
    });

    QElapsedTimer elapsed;
    elapsed.start();
    while (received.count() == 0 && !elapsed.hasExpired(1'000)) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QTest::qWait(10);
    }
    callbackProcess.join();
    QCOMPARE(received.count(), 1);
    QVERIFY2(result == AniListOAuthCallbackReceiver::StartResult::Forwarded, qPrintable(forwardingError));
    QCOMPARE(received.first().first().toUrl(), callback);
}

QTEST_MAIN(AniListOAuthCallbackReceiverTests)
#include "AniListOAuthCallbackReceiverTests.moc"
