#ifndef HAIKENANIME_PLEXTESTSUPPORT_H
#define HAIKENANIME_PLEXTESTSUPPORT_H

#include <QFile>
#include <QHash>
#include <QList>
#include <QMutex>
#include <QStringList>
#include <QUrlQuery>

#include <map>
#include <utility>

#include "../../src/application/recognition/IPluginSecretStore.h"
#include "../../src/infrastructure/recognition/plex/PlexHttp.h"

/// Answers by "<METHOD> <scheme://host/path>" (query ignored); unknown routes return 404.
class FakePlexHttp final : public IPlexHttp {
public:
    void send(const PlexHttpRequest &request, std::function<void(const PlexHttpResponse &)> completion) override {
        requests.append(request);
        auto url = request.url;
        url.setQuery(QString());
        const auto key = QString::fromLatin1(request.method) + QLatin1Char(' ') + url.toString();
        auto &queue = routes[key];
        if (queue.isEmpty()) {
            completion({.status = 404});
            return;
        }
        const auto response = queue.size() > 1 ? queue.takeFirst() : queue.first();
        completion(response);
    }

    void route(const QString &key, PlexHttpResponse response) { routes[key].append(std::move(response)); }

    [[nodiscard]] QByteArray header(const PlexHttpRequest &request, const QByteArray &name) const {
        for (const auto &[headerName, value] : request.headers) {
            if (headerName.compare(name, Qt::CaseInsensitive) == 0) return value;
        }
        return {};
    }

    QHash<QString, QList<PlexHttpResponse>> routes;
    QList<PlexHttpRequest> requests;
};

class FakeSecretStore final : public IPluginSecretStore {
public:
    bool hasSecret(const QString &pluginId, const QString &key) const override {
        return secrets.contains({pluginId, key});
    }
    bool readSecret(const QString &pluginId, const QString &key, QString &value, QString &error) override {
        const auto found = secrets.find({pluginId, key});
        if (found == secrets.end()) {
            error = QStringLiteral("missing");
            return false;
        }
        value = found->second;
        return true;
    }
    bool writeSecret(const QString &pluginId, const QString &key, const QString &value, QString &) override {
        secrets[{pluginId, key}] = value;
        return true;
    }
    bool clearSecret(const QString &pluginId, const QString &key, QString &) override {
        secrets.erase({pluginId, key});
        return true;
    }

    std::map<std::pair<QString, QString>, QString> secrets;
};

/// Captures every Qt log message emitted while alive.
class LogCapture final {
public:
    LogCapture() {
        active() = this;
        previous_ = qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &message) {
            if (active()) active()->messages.append(message);
        });
    }
    ~LogCapture() {
        qInstallMessageHandler(previous_);
        active() = nullptr;
    }

    [[nodiscard]] bool contains(const QString &text) const {
        for (const auto &message : messages) {
            if (message.contains(text)) return true;
        }
        return false;
    }

    QStringList messages;

private:
    static LogCapture *&active() {
        static LogCapture *instance = nullptr;
        return instance;
    }

    QtMessageHandler previous_ = nullptr;
};

inline QByteArray PlexFixture(const QString &name) {
    QFile file(QStringLiteral(HAIKENANIME_TEST_SOURCE_DIR "/tests/fixtures/") + name);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return file.readAll();
}

#endif // HAIKENANIME_PLEXTESTSUPPORT_H
