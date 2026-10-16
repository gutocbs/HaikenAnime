#include <QtTest>

#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryFile>

#include "../../src/infrastructure/anilist/AniListGraphQlClient.h"
#include "../../src/infrastructure/anilist/AniListUpdateClient.h"
#include "../../src/infrastructure/anilist/GraphQlQueryStore.h"

namespace {
class RecordingGraphQlServer final {
public:
    bool start() {
        QObject::connect(&server_, &QTcpServer::newConnection, &server_, [this] {
            auto *socket = server_.nextPendingConnection();
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [this, socket] {
                auto &request = requests_[socket];
                request += socket->readAll();
                const auto headerEnd = request.indexOf(QByteArrayLiteral("\r\n\r\n"));
                if (headerEnd < 0 || replied_.contains(socket)) return;
                replied_.insert(socket);
                payloads_.append(QJsonDocument::fromJson(request.mid(headerEnd + 4)).object());
                QByteArray body = R"json({"data":{"SaveMediaListEntry":{"id":1}}})json";
                QByteArray status = QByteArrayLiteral("200 OK");
                if (!responses_.isEmpty()) {
                    const auto next = responses_.takeFirst();
                    body = next.body;
                    status = next.status;
                }
                const QByteArray response = QByteArrayLiteral("HTTP/1.1 ") + status
                    + QByteArrayLiteral("\r\nContent-Type: application/json\r\nContent-Length: ")
                    + QByteArray::number(body.size())
                    + QByteArrayLiteral("\r\nConnection: close\r\n\r\n") + body;
                socket->write(response);
                socket->disconnectFromHost();
            });
        });
        return server_.listen(QHostAddress::LocalHost);
    }

    [[nodiscard]] QUrl endpoint() const {
        return QUrl(QStringLiteral("http://127.0.0.1:%1/graphql").arg(server_.serverPort()));
    }

    struct Reply {
        QByteArray body;
        QByteArray status = QByteArrayLiteral("200 OK");
    };

    QList<QJsonObject> payloads_;
    QList<Reply> responses_;

private:
    QTcpServer server_;
    QHash<QTcpSocket *, QByteArray> requests_;
    QSet<QTcpSocket *> replied_;
};

AniListPendingChange change(const AniListField field, AniListFieldValue value) {
    AniListPendingChange result;
    result.mediaId = 154587;
    result.field = field;
    result.newValue = std::move(value);
    return result;
}
}

class AniListUpdateClientTests final : public QObject {
    Q_OBJECT

private slots:
    void sendsOneMutationWithEveryChangedFieldForOneMedia();
    void deletesTheResolvedListEntryId();
    void treatsMissingListEntryAsAlreadyRemoved();
    void treatsNotFoundGraphQlErrorAsAlreadyRemoved();
    void failsWhenDeletionIsNotConfirmed();
    void rejectsDeletionWithoutAccountId();
};

namespace {
struct DeletionHarness {
    explicit DeletionHarness(QList<RecordingGraphQlServer::Reply> replies) {
        server.responses_ = std::move(replies);
        started = server.start();
        queryFile.open();
        queryFile.write("query Q($userId: Int!, $mediaId: Int!) { MediaList(userId: $userId, mediaId: $mediaId) { id } }");
        queryFile.flush();
        mutationFile.open();
        mutationFile.write("mutation M($id: Int!) { DeleteMediaListEntry(id: $id) { deleted } }");
        mutationFile.flush();
    }

    RecordingGraphQlServer server;
    QTemporaryFile queryFile;
    QTemporaryFile mutationFile;
    bool started = false;
};

}

void AniListUpdateClientTests::sendsOneMutationWithEveryChangedFieldForOneMedia() {
    RecordingGraphQlServer server;
    QVERIFY(server.start());
    QTemporaryFile mutationFile;
    QVERIFY(mutationFile.open());
    mutationFile.write(R"graphql(
        mutation UpdateMedia($mediaId: Int!, $progress: Int, $score: Float, $status: MediaListStatus) {
          SaveMediaListEntry(mediaId: $mediaId, progress: $progress, score: $score, status: $status) { id }
        }
    )graphql");
    mutationFile.flush();

    QNetworkAccessManager networkManager;
    AniListGraphQlClient graphQlClient(networkManager, nullptr, server.endpoint(), 1000);
    GraphQlQueryStore updateMutation(mutationFile.fileName());
    GraphQlQueryStore entryIdQuery(mutationFile.fileName());
    GraphQlQueryStore deleteMutation(mutationFile.fileName());
    AniListUpdateClient client(graphQlClient, updateMutation, entryIdQuery, deleteMutation);
    AniListMediaPendingChanges changes{
        .mediaId = 154587,
        .changes = {
            change(AniListField::Progress, 12),
            change(AniListField::PersonalScore, 9),
            change(AniListField::ListStatus, QStringLiteral("COMPLETED"))}};
    QString error;

    QVERIFY2(client.updateMedia(changes, error), qPrintable(error));
    QCOMPARE(server.payloads_.size(), 1);
    const auto variables = server.payloads_.first().value(QStringLiteral("variables")).toObject();
    QCOMPARE(variables.value(QStringLiteral("mediaId")).toInt(), 154587);
    QCOMPARE(variables.value(QStringLiteral("progress")).toInt(), 12);
    QCOMPARE(variables.value(QStringLiteral("score")).toInt(), 9);
    QCOMPARE(variables.value(QStringLiteral("status")).toString(), QStringLiteral("COMPLETED"));
}

namespace {
using Reply = RecordingGraphQlServer::Reply;
const QByteArray kEntryFound = R"json({"data":{"MediaList":{"id":987}}})json";
const QByteArray kDeleted = R"json({"data":{"DeleteMediaListEntry":{"deleted":true}}})json";
}

void AniListUpdateClientTests::deletesTheResolvedListEntryId() {
    DeletionHarness harness({Reply{kEntryFound}, Reply{kDeleted}});
    QVERIFY(harness.started);
    QNetworkAccessManager networkManager;
    AniListGraphQlClient graphQlClient(networkManager, nullptr, harness.server.endpoint(), 1000);
    GraphQlQueryStore update(harness.queryFile.fileName());
    GraphQlQueryStore entryIdQuery(harness.queryFile.fileName());
    GraphQlQueryStore deleteMutation(harness.mutationFile.fileName());
    AniListUpdateClient client(graphQlClient, update, entryIdQuery, deleteMutation);
    QString error;

    QVERIFY2(client.deleteMediaListEntry(154587, 42, error), qPrintable(error));
    QCOMPARE(harness.server.payloads_.size(), 2);
    const auto lookup = harness.server.payloads_.at(0).value(QStringLiteral("variables")).toObject();
    QCOMPARE(lookup.value(QStringLiteral("userId")).toInt(), 42);
    QCOMPARE(lookup.value(QStringLiteral("mediaId")).toInt(), 154587);
    const auto removal = harness.server.payloads_.at(1).value(QStringLiteral("variables")).toObject();
    QCOMPARE(removal.value(QStringLiteral("id")).toInt(), 987);
}

void AniListUpdateClientTests::treatsMissingListEntryAsAlreadyRemoved() {
    DeletionHarness harness({Reply{R"json({"data":{"MediaList":null}})json"}});
    QVERIFY(harness.started);
    QNetworkAccessManager networkManager;
    AniListGraphQlClient graphQlClient(networkManager, nullptr, harness.server.endpoint(), 1000);
    GraphQlQueryStore update(harness.queryFile.fileName());
    GraphQlQueryStore entryIdQuery(harness.queryFile.fileName());
    GraphQlQueryStore deleteMutation(harness.mutationFile.fileName());
    AniListUpdateClient client(graphQlClient, update, entryIdQuery, deleteMutation);
    QString error;

    QVERIFY2(client.deleteMediaListEntry(154587, 42, error), qPrintable(error));
    QCOMPARE(harness.server.payloads_.size(), 1);
}

void AniListUpdateClientTests::treatsNotFoundGraphQlErrorAsAlreadyRemoved() {
    DeletionHarness harness({Reply{
        R"json({"errors":[{"message":"Not Found.","status":404}],"data":{"MediaList":null}})json",
        QByteArrayLiteral("404 Not Found")}});
    QVERIFY(harness.started);
    QNetworkAccessManager networkManager;
    AniListGraphQlClient graphQlClient(networkManager, nullptr, harness.server.endpoint(), 1000);
    GraphQlQueryStore update(harness.queryFile.fileName());
    GraphQlQueryStore entryIdQuery(harness.queryFile.fileName());
    GraphQlQueryStore deleteMutation(harness.mutationFile.fileName());
    AniListUpdateClient client(graphQlClient, update, entryIdQuery, deleteMutation);
    QString error;

    QVERIFY2(client.deleteMediaListEntry(154587, 42, error), qPrintable(error));
    QVERIFY(error.isEmpty());
    QCOMPARE(harness.server.payloads_.size(), 1);
}

void AniListUpdateClientTests::failsWhenDeletionIsNotConfirmed() {
    DeletionHarness harness({Reply{kEntryFound},
        Reply{R"json({"data":{"DeleteMediaListEntry":{"deleted":false}}})json"}});
    QVERIFY(harness.started);
    QNetworkAccessManager networkManager;
    AniListGraphQlClient graphQlClient(networkManager, nullptr, harness.server.endpoint(), 1000);
    GraphQlQueryStore update(harness.queryFile.fileName());
    GraphQlQueryStore entryIdQuery(harness.queryFile.fileName());
    GraphQlQueryStore deleteMutation(harness.mutationFile.fileName());
    AniListUpdateClient client(graphQlClient, update, entryIdQuery, deleteMutation);
    QString error;

    QVERIFY(!client.deleteMediaListEntry(154587, 42, error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(harness.server.payloads_.size(), 2);
}

void AniListUpdateClientTests::rejectsDeletionWithoutAccountId() {
    DeletionHarness harness({});
    QVERIFY(harness.started);
    QNetworkAccessManager networkManager;
    AniListGraphQlClient graphQlClient(networkManager, nullptr, harness.server.endpoint(), 1000);
    GraphQlQueryStore update(harness.queryFile.fileName());
    GraphQlQueryStore entryIdQuery(harness.queryFile.fileName());
    GraphQlQueryStore deleteMutation(harness.mutationFile.fileName());
    AniListUpdateClient client(graphQlClient, update, entryIdQuery, deleteMutation);
    QString error;

    QVERIFY(!client.deleteMediaListEntry(154587, 0, error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(harness.server.payloads_.size(), 0);
}

QTEST_MAIN(AniListUpdateClientTests)
#include "AniListUpdateClientTests.moc"
