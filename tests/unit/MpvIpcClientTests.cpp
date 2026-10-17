#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QUuid>
#include <QtTest>

#include "../../src/infrastructure/recognition/mpv/MpvIpcClient.h"
#include "../../src/infrastructure/recognition/mpv/QtLocalSocketMpvTransport.h"

namespace {
class FakeTransport final : public IMpvTransport {
public:
    void open(const QString &pipeName, std::function<void(bool)> completion) override {
        openedPipe = pipeName;
        completion(accept);
    }
    void write(const QByteArray &bytes) override { written.append(bytes); }
    void close() override { closed = true; }
    void setReadHandler(std::function<void(const QByteArray &)> handler) override { read = std::move(handler); }
    void setDisconnectedHandler(std::function<void()> handler) override { disconnected = std::move(handler); }

    void deliver(const QByteArray &bytes) { read(bytes); }

    QJsonObject lastCommand() const {
        return QJsonDocument::fromJson(written.last().trimmed()).object();
    }

    bool accept = true;
    QString openedPipe;
    QList<QByteArray> written;
    bool closed = false;
    std::function<void(const QByteArray &)> read;
    std::function<void()> disconnected;
};

QByteArray fixture(const QString &name) {
    QFile file(QStringLiteral(HAIKENANIME_TEST_SOURCE_DIR "/tests/fixtures/") + name);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return file.readAll();
}

struct Events {
    QList<QPair<QString, QJsonValue>> properties;
    QStringList endReasons;
    int shutdowns = 0;
    int disconnects = 0;
    QStringList protocolErrors;

    MpvIpcCallbacks callbacks() {
        return {.propertyChanged = [this](const QString &name, const QJsonValue &data) { properties.append({name, data}); },
                .fileEnded = [this](const QString &reason) { endReasons.append(reason); },
                .shutdown = [this] { ++shutdowns; },
                .disconnected = [this] { ++disconnects; },
                .protocolError = [this](const QString &message) { protocolErrors.append(message); }};
    }
};
}

class MpvIpcClientTests final : public QObject {
    Q_OBJECT

private slots:
    void connectOpensThePipeAndReportsResult();
    void getPropertyWritesNewlineTerminatedCommandWithRequestId();
    void getPropertyReplyDeliversData();
    void getPropertyErrorReplyDeliversError();
    void observePropertyWritesCommand();
    void dispatchesPropertyChangesFromPlayingFixture();
    void handlesMessagesSplitAcrossAndPackedInChunks();
    void skipsMalformedJsonAndKeepsProcessing();
    void dispatchesEndFileAndShutdownFromFixture();
    void interleavedEventsDoNotConsumeReplies();
    void transportDisconnectFailsPendingRepliesAndNotifies();
    void qtTransportTalksToARealNamedPipe();
};

void MpvIpcClientTests::connectOpensThePipeAndReportsResult() {
    FakeTransport transport;
    Events events;
    MpvIpcClient client(transport, events.callbacks());
    bool connected = false;

    client.connect(QStringLiteral("haikenanime-mpv-1"), [&connected](const bool value) { connected = value; });

    QVERIFY(connected);
    QVERIFY(client.isConnected());
    QCOMPARE(transport.openedPipe, QStringLiteral("haikenanime-mpv-1"));

    FakeTransport refusing;
    refusing.accept = false;
    MpvIpcClient refused(refusing, events.callbacks());
    bool result = true;
    refused.connect(QStringLiteral("missing"), [&result](const bool value) { result = value; });
    QVERIFY(!result);
    QVERIFY(!refused.isConnected());
}

void MpvIpcClientTests::getPropertyWritesNewlineTerminatedCommandWithRequestId() {
    FakeTransport transport;
    Events events;
    MpvIpcClient client(transport, events.callbacks());
    client.connect(QStringLiteral("p"), [](bool) {});

    client.getProperty(QStringLiteral("path"), [](bool, const QJsonValue &, const QString &) {});

    QCOMPARE(transport.written.size(), 1);
    QVERIFY(transport.written.first().endsWith('\n'));
    QCOMPARE(transport.written.first().count('\n'), 1);
    const auto command = transport.lastCommand();
    QCOMPARE(command.value(QStringLiteral("command")).toArray(),
             QJsonArray({QStringLiteral("get_property"), QStringLiteral("path")}));
    QVERIFY(command.value(QStringLiteral("request_id")).toInteger() > 0);
}

void MpvIpcClientTests::getPropertyReplyDeliversData() {
    FakeTransport transport;
    Events events;
    MpvIpcClient client(transport, events.callbacks());
    client.connect(QStringLiteral("p"), [](bool) {});
    QString path;
    client.getProperty(QStringLiteral("path"), [&path](const bool ok, const QJsonValue &data, const QString &) {
        if (ok) path = data.toString();
    });
    const auto requestId = transport.lastCommand().value(QStringLiteral("request_id")).toInteger();

    transport.deliver(QStringLiteral(R"({"error":"success","data":"D:\\Anime\\ep01.mkv","request_id":%1})")
                          .arg(requestId).toUtf8() + '\n');

    QCOMPARE(path, QStringLiteral("D:\\Anime\\ep01.mkv"));
}

void MpvIpcClientTests::getPropertyErrorReplyDeliversError() {
    FakeTransport transport;
    Events events;
    MpvIpcClient client(transport, events.callbacks());
    client.connect(QStringLiteral("p"), [](bool) {});
    bool succeeded = true;
    QString error;
    client.getProperty(QStringLiteral("duration"),
                       [&](const bool ok, const QJsonValue &, const QString &message) {
                           succeeded = ok;
                           error = message;
                       });
    const auto requestId = transport.lastCommand().value(QStringLiteral("request_id")).toInteger();

    transport.deliver(QStringLiteral(R"({"error":"property unavailable","request_id":%1})").arg(requestId).toUtf8()
                      + '\n');

    QVERIFY(!succeeded);
    QCOMPARE(error, QStringLiteral("property unavailable"));
}

void MpvIpcClientTests::observePropertyWritesCommand() {
    FakeTransport transport;
    Events events;
    MpvIpcClient client(transport, events.callbacks());
    client.connect(QStringLiteral("p"), [](bool) {});

    client.observeProperty(2, QStringLiteral("time-pos"));

    QCOMPARE(transport.lastCommand().value(QStringLiteral("command")).toArray(),
             QJsonArray({QStringLiteral("observe_property"), 2, QStringLiteral("time-pos")}));
}

void MpvIpcClientTests::dispatchesPropertyChangesFromPlayingFixture() {
    FakeTransport transport;
    Events events;
    MpvIpcClient client(transport, events.callbacks());
    client.connect(QStringLiteral("p"), [](bool) {});

    transport.deliver(fixture(QStringLiteral("mpv-playing.json")));

    QCOMPARE(events.properties.size(), 4);
    QCOMPARE(events.properties.at(0).first, QStringLiteral("pause"));
    QCOMPARE(events.properties.at(0).second.toBool(), false);
    QCOMPARE(events.properties.at(1).second.toString(),
             QStringLiteral("D:\\Anime\\One Piece\\[Group] One Piece - 01.mkv"));
    QCOMPARE(events.properties.at(2).first, QStringLiteral("time-pos"));
    QCOMPARE(events.properties.at(2).second.toDouble(), 12.5);
    QCOMPARE(events.properties.at(3).second.toBool(), true);
    QVERIFY(events.protocolErrors.isEmpty());
}

void MpvIpcClientTests::handlesMessagesSplitAcrossAndPackedInChunks() {
    FakeTransport transport;
    Events events;
    MpvIpcClient client(transport, events.callbacks());
    client.connect(QStringLiteral("p"), [](bool) {});

    transport.deliver(R"({"event":"property-change","id":2,"name":"time-)");
    QVERIFY(events.properties.isEmpty());
    transport.deliver("pos\",\"data\":1.0}\n{\"event\":\"property-change\",\"id\":2,\"name\":\"time-pos\",\"data\":2.0}\n");

    QCOMPARE(events.properties.size(), 2);
    QCOMPARE(events.properties.at(1).second.toDouble(), 2.0);
}

void MpvIpcClientTests::skipsMalformedJsonAndKeepsProcessing() {
    FakeTransport transport;
    Events events;
    MpvIpcClient client(transport, events.callbacks());
    client.connect(QStringLiteral("p"), [](bool) {});

    transport.deliver("{not json\n[1,2]\n\xff\xfe\n{\"event\":\"property-change\",\"id\":1,\"name\":\"pause\",\"data\":true}\n");

    QCOMPARE(events.protocolErrors.size(), 3);
    QCOMPARE(events.properties.size(), 1);
}

void MpvIpcClientTests::dispatchesEndFileAndShutdownFromFixture() {
    FakeTransport transport;
    Events events;
    MpvIpcClient client(transport, events.callbacks());
    client.connect(QStringLiteral("p"), [](bool) {});

    transport.deliver(fixture(QStringLiteral("mpv-shutdown.json")));

    QCOMPARE(events.endReasons, QStringList({QStringLiteral("quit")}));
    QCOMPARE(events.shutdowns, 1);
}

void MpvIpcClientTests::interleavedEventsDoNotConsumeReplies() {
    FakeTransport transport;
    Events events;
    MpvIpcClient client(transport, events.callbacks());
    client.connect(QStringLiteral("p"), [](bool) {});
    double position = -1;
    client.getProperty(QStringLiteral("time-pos"), [&position](const bool ok, const QJsonValue &data, const QString &) {
        if (ok) position = data.toDouble();
    });
    const auto requestId = transport.lastCommand().value(QStringLiteral("request_id")).toInteger();

    transport.deliver("{\"event\":\"playback-restart\"}\n");
    QCOMPARE(position, -1.0);
    transport.deliver(QStringLiteral(R"({"request_id":%1,"error":"success","data":42.0})").arg(requestId).toUtf8() + '\n');

    QCOMPARE(position, 42.0);
}

void MpvIpcClientTests::transportDisconnectFailsPendingRepliesAndNotifies() {
    FakeTransport transport;
    Events events;
    MpvIpcClient client(transport, events.callbacks());
    client.connect(QStringLiteral("p"), [](bool) {});
    bool replied = false;
    bool succeeded = true;
    client.getProperty(QStringLiteral("path"), [&](const bool ok, const QJsonValue &, const QString &) {
        replied = true;
        succeeded = ok;
    });

    transport.disconnected();

    QVERIFY(replied);
    QVERIFY(!succeeded);
    QCOMPARE(events.disconnects, 1);
    QVERIFY(!client.isConnected());
}

void MpvIpcClientTests::qtTransportTalksToARealNamedPipe() {
    const auto pipeName = QStringLiteral("haikenanime-test-%1").arg(QUuid::createUuid().toString(QUuid::Id128));
    QLocalServer server;
    QVERIFY2(server.listen(pipeName), qPrintable(server.errorString()));
    QtLocalSocketMpvTransport transport;
    QByteArray received;
    transport.setReadHandler([&received](const QByteArray &chunk) { received += chunk; });
    bool disconnected = false;
    transport.setDisconnectedHandler([&disconnected] { disconnected = true; });
    std::optional<bool> connected;

    transport.open(pipeName, [&connected](const bool value) { connected = value; });
    QVERIFY(server.waitForNewConnection(5000));
    QTRY_VERIFY(connected.has_value());
    QVERIFY(*connected);
    QLocalSocket *peer = server.nextPendingConnection();
    transport.write("{\"command\":[\"get_property\",\"path\"]}\n");
    QTRY_VERIFY(peer->bytesAvailable() > 0 || peer->waitForReadyRead(100));
    QCOMPARE(peer->readAll(), QByteArray("{\"command\":[\"get_property\",\"path\"]}\n"));
    peer->write("{\"event\":\"shutdown\"}\n");
    peer->flush();
    QTRY_COMPARE(received, QByteArray("{\"event\":\"shutdown\"}\n"));
    peer->disconnectFromServer();
    QTRY_VERIFY(disconnected);

    QtLocalSocketMpvTransport missing;
    std::optional<bool> missingResult;
    missing.open(QStringLiteral("haikenanime-test-missing-pipe"), [&missingResult](const bool value) {
        missingResult = value;
    });
    QTRY_VERIFY(missingResult.has_value());
    QVERIFY(!*missingResult);
}

QTEST_MAIN(MpvIpcClientTests)
#include "MpvIpcClientTests.moc"
