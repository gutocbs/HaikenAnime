#ifndef HAIKENANIME_IMPVTRANSPORT_H
#define HAIKENANIME_IMPVTRANSPORT_H

#include <QByteArray>
#include <QString>

#include <functional>

/// Byte stream to one mpv JSON IPC endpoint (a Windows named pipe in production).
/// Replaceable so the protocol can be tested without a running player.
class IMpvTransport {
public:
    virtual ~IMpvTransport() = default;

    /// Connects to the pipe name (without the \\.\pipe\ prefix); completion reports success.
    virtual void open(const QString &pipeName, std::function<void(bool connected)> completion) = 0;
    virtual void write(const QByteArray &bytes) = 0;
    virtual void close() = 0;
    /// Receives raw chunks; messages may be split across or packed within chunks.
    virtual void setReadHandler(std::function<void(const QByteArray &)> handler) = 0;
    virtual void setDisconnectedHandler(std::function<void()> handler) = 0;
};

#endif // HAIKENANIME_IMPVTRANSPORT_H
