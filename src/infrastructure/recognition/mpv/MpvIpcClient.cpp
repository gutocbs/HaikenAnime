#include "MpvIpcClient.h"

#include <QJsonDocument>
#include <QJsonObject>

#include <utility>

MpvIpcClient::MpvIpcClient(IMpvTransport &transport, MpvIpcCallbacks callbacks)
    : transport_(transport), callbacks_(std::move(callbacks)) {
    transport_.setReadHandler([this](const QByteArray &chunk) { receive(chunk); });
    transport_.setDisconnectedHandler([this] {
        connected_ = false;
        // Replies that will never arrive are failed so callers are not left waiting.
        const auto pending = std::exchange(pending_, {});
        for (const auto &handler : pending) handler(false, {}, QStringLiteral("disconnected"));
        if (callbacks_.disconnected) callbacks_.disconnected();
    });
}

MpvIpcClient::~MpvIpcClient() {
    transport_.setReadHandler({});
    transport_.setDisconnectedHandler({});
}

void MpvIpcClient::connect(const QString &pipeName, std::function<void(bool)> completion) {
    transport_.open(pipeName, [this, completion = std::move(completion)](const bool connected) {
        connected_ = connected;
        completion(connected);
    });
}

void MpvIpcClient::getProperty(const QString &name, ReplyHandler handler) {
    send(QJsonArray{QStringLiteral("get_property"), name}, std::move(handler));
}

void MpvIpcClient::observeProperty(const int id, const QString &name) {
    send(QJsonArray{QStringLiteral("observe_property"), id, name}, {});
}

void MpvIpcClient::close() {
    connected_ = false;
    pending_.clear();
    transport_.close();
}

void MpvIpcClient::send(const QJsonArray &command, ReplyHandler handler) {
    const auto requestId = nextRequestId_++;
    if (handler) pending_.insert(requestId, std::move(handler));
    const QJsonObject message{{QStringLiteral("command"), command}, {QStringLiteral("request_id"), requestId}};
    // One compact JSON object per line: mpv rejects messages containing raw newlines.
    transport_.write(QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n');
}

void MpvIpcClient::receive(const QByteArray &chunk) {
    buffer_.append(chunk);
    qsizetype newline = buffer_.indexOf('\n');
    while (newline >= 0) {
        const auto line = buffer_.left(newline).trimmed();
        buffer_.remove(0, newline + 1);
        if (!line.isEmpty()) dispatch(line);
        newline = buffer_.indexOf('\n');
    }
}

void MpvIpcClient::dispatch(const QByteArray &line) {
    // mpv may emit invalid UTF-8 (file names); decoding replaces it before JSON parsing.
    const auto sanitized = QString::fromUtf8(line).toUtf8();
    QJsonParseError parseError{};
    const auto document = QJsonDocument::fromJson(sanitized, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (callbacks_.protocolError) callbacks_.protocolError(QStringLiteral("mpv sent a line that is not a JSON object."));
        return;
    }
    const auto message = document.object();
    const auto event = message.value(QStringLiteral("event")).toString();
    if (!event.isEmpty()) {
        if (event == QStringLiteral("property-change")) {
            if (callbacks_.propertyChanged) {
                callbacks_.propertyChanged(message.value(QStringLiteral("name")).toString(),
                                           message.value(QStringLiteral("data")));
            }
        } else if (event == QStringLiteral("end-file")) {
            if (callbacks_.fileEnded) callbacks_.fileEnded(message.value(QStringLiteral("reason")).toString());
        } else if (event == QStringLiteral("shutdown")) {
            if (callbacks_.shutdown) callbacks_.shutdown();
        }
        return;
    }
    const auto found = pending_.find(message.value(QStringLiteral("request_id")).toInteger());
    if (found == pending_.end()) return;
    const auto handler = std::move(found.value());
    pending_.erase(found);
    const auto error = message.value(QStringLiteral("error")).toString();
    handler(error == QStringLiteral("success"), message.value(QStringLiteral("data")), error);
}
