#include "AniListOAuthCallbackReceiver.h"

#include <QLocalSocket>

#include <utility>

AniListOAuthCallbackReceiver::AniListOAuthCallbackReceiver(QString serverName, QObject *parent)
    : QObject(parent), serverName_(std::move(serverName)) {
    connect(&server_, &QLocalServer::newConnection, this,
            &AniListOAuthCallbackReceiver::receivePendingConnections);
}

AniListOAuthCallbackReceiver::~AniListOAuthCallbackReceiver() {
    if (!server_.isListening()) return;
    server_.close();
    QLocalServer::removeServer(serverName_);
}

std::optional<QUrl> AniListOAuthCallbackReceiver::callbackFromArguments(
    const QStringList &arguments) {
    for (const auto &argument : arguments) {
        const QUrl candidate(argument);
        if (candidate.isValid() && candidate.scheme() == QStringLiteral("haikenanime")) {
            return candidate;
        }
    }
    return std::nullopt;
}

AniListOAuthCallbackReceiver::StartResult AniListOAuthCallbackReceiver::start(
    const QStringList &arguments, QString &error) {
    error.clear();
    const auto callback = callbackFromArguments(arguments);
    if (callback.has_value()) {
        QLocalSocket socket;
        socket.connectToServer(serverName_);
        if (socket.waitForConnected(1000)) {
            socket.write(callback->toString(QUrl::FullyEncoded).toUtf8());
            if (!socket.waitForBytesWritten(1000)) {
                error = socket.errorString();
                return StartResult::Unavailable;
            }
            return StartResult::Forwarded;
        }
    }

    server_.setSocketOptions(QLocalServer::UserAccessOption);
    if (server_.listen(serverName_)) return StartResult::Listening;

    error = server_.errorString();
    return StartResult::Unavailable;
}

void AniListOAuthCallbackReceiver::receivePendingConnections() {
    while (QLocalSocket *socket = server_.nextPendingConnection()) {
        socket->setParent(this);
        connect(socket, &QLocalSocket::readyRead, this, [this, socket] {
            const QUrl callback(QString::fromUtf8(socket->readAll()));
            if (callback.isValid() && callback.scheme() == QStringLiteral("haikenanime")) {
                emit callbackReceived(callback);
            }
            socket->disconnectFromServer();
            socket->deleteLater();
        });
    }
}
