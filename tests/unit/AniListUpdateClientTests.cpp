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
                const QByteArray body = R"json({"data":{"SaveMediaListEntry":{"id":1}}})json";
                const QByteArray response = QByteArrayLiteral(
                    "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: ")
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

    QList<QJsonObject> payloads_;

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
};

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
    AniListUpdateClient client(graphQlClient, updateMutation);
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

QTEST_MAIN(AniListUpdateClientTests)
#include "AniListUpdateClientTests.moc"
