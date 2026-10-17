#ifndef HAIKENANIME_MPVIPCCLIENT_H
#define HAIKENANIME_MPVIPCCLIENT_H

#include <QByteArray>
#include <QHash>
#include <QJsonArray>
#include <QJsonValue>
#include <QString>

#include <functional>

#include "IMpvTransport.h"

struct MpvIpcCallbacks final {
    std::function<void(const QString &name, const QJsonValue &data)> propertyChanged;
    /// end-file event; reason is one of eof, stop, quit, error, redirect, unknown.
    std::function<void(const QString &reason)> fileEnded;
    std::function<void()> shutdown;
    std::function<void()> disconnected;
    /// A line that is not a JSON object was skipped; the message never contains file content.
    std::function<void(const QString &safeMessage)> protocolError;
};

/// mpv JSON IPC protocol: newline-delimited JSON commands with request ids, replies and events.
/// See docs/superpowers/references/2026-10-17-mpv-plex-contratos.md.
class MpvIpcClient final {
public:
    using ReplyHandler = std::function<void(bool succeeded, const QJsonValue &data, const QString &error)>;

    MpvIpcClient(IMpvTransport &transport, MpvIpcCallbacks callbacks);
    ~MpvIpcClient();

    void connect(const QString &pipeName, std::function<void(bool connected)> completion);
    void getProperty(const QString &name, ReplyHandler handler);
    void observeProperty(int id, const QString &name);
    void close();
    [[nodiscard]] bool isConnected() const { return connected_; }

private:
    void send(const QJsonArray &command, ReplyHandler handler);
    void receive(const QByteArray &chunk);
    void dispatch(const QByteArray &line);

    IMpvTransport &transport_;
    MpvIpcCallbacks callbacks_;
    QByteArray buffer_;
    QHash<qint64, ReplyHandler> pending_;
    qint64 nextRequestId_ = 1;
    bool connected_ = false;
};

#endif // HAIKENANIME_MPVIPCCLIENT_H
