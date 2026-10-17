#ifndef HAIKENANIME_QTLOCALSOCKETMPVTRANSPORT_H
#define HAIKENANIME_QTLOCALSOCKETMPVTRANSPORT_H

#include <QLocalSocket>
#include <QObject>

#include "IMpvTransport.h"

/// QLocalSocket maps to Windows named pipes (\\.\pipe\<name>), which is what mpv's
/// --input-ipc-server creates on Windows.
class QtLocalSocketMpvTransport final : public QObject, public IMpvTransport {
    Q_OBJECT

public:
    explicit QtLocalSocketMpvTransport(QObject *parent = nullptr);
    ~QtLocalSocketMpvTransport() override;

    void open(const QString &pipeName, std::function<void(bool connected)> completion) override;
    void write(const QByteArray &bytes) override;
    void close() override;
    void setReadHandler(std::function<void(const QByteArray &)> handler) override;
    void setDisconnectedHandler(std::function<void()> handler) override;

private:
    QLocalSocket socket_;
    std::function<void(bool)> openCompletion_;
    std::function<void(const QByteArray &)> readHandler_;
    std::function<void()> disconnectedHandler_;
    bool closing_ = false;
};

#endif // HAIKENANIME_QTLOCALSOCKETMPVTRANSPORT_H
