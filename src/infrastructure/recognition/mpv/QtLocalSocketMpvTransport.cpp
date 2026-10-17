#include "QtLocalSocketMpvTransport.h"

#include <utility>

QtLocalSocketMpvTransport::QtLocalSocketMpvTransport(QObject *parent) : QObject(parent) {
    connect(&socket_, &QLocalSocket::connected, this, [this] {
        if (auto completion = std::exchange(openCompletion_, {})) completion(true);
    });
    connect(&socket_, &QLocalSocket::errorOccurred, this, [this](QLocalSocket::LocalSocketError) {
        if (auto completion = std::exchange(openCompletion_, {})) completion(false);
    });
    connect(&socket_, &QLocalSocket::readyRead, this, [this] {
        const auto bytes = socket_.readAll();
        if (readHandler_) readHandler_(bytes);
    });
    connect(&socket_, &QLocalSocket::disconnected, this, [this] {
        if (!closing_ && disconnectedHandler_) disconnectedHandler_();
    });
}

QtLocalSocketMpvTransport::~QtLocalSocketMpvTransport() {
    closing_ = true;
    socket_.disconnect(this);
    socket_.abort();
}

void QtLocalSocketMpvTransport::open(const QString &pipeName, std::function<void(bool)> completion) {
    closing_ = false;
    openCompletion_ = std::move(completion);
    // On Windows QLocalSocket resolves a bare name to \\.\pipe\<name>.
    socket_.connectToServer(pipeName);
}

void QtLocalSocketMpvTransport::write(const QByteArray &bytes) {
    socket_.write(bytes);
    socket_.flush();
}

void QtLocalSocketMpvTransport::close() {
    closing_ = true;
    socket_.abort();
}

void QtLocalSocketMpvTransport::setReadHandler(std::function<void(const QByteArray &)> handler) {
    readHandler_ = std::move(handler);
}

void QtLocalSocketMpvTransport::setDisconnectedHandler(std::function<void()> handler) {
    disconnectedHandler_ = std::move(handler);
}
