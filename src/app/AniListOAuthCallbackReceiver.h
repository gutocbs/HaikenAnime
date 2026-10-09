#ifndef HAIKENANIME_ANILISTOAUTHCALLBACKRECEIVER_H
#define HAIKENANIME_ANILISTOAUTHCALLBACKRECEIVER_H

#include <QObject>
#include <QLocalServer>
#include <QUrl>

#include <optional>

/** Routes an AniList custom-scheme callback to the running application instance. */
class AniListOAuthCallbackReceiver final : public QObject {
    Q_OBJECT
public:
    enum class StartResult { Listening, Forwarded, Unavailable };

    explicit AniListOAuthCallbackReceiver(QString serverName, QObject *parent = nullptr);
    ~AniListOAuthCallbackReceiver() override;

    [[nodiscard]] static std::optional<QUrl> callbackFromArguments(const QStringList &arguments);
    [[nodiscard]] StartResult start(const QStringList &arguments, QString &error);

signals:
    void callbackReceived(QUrl callback);

private:
    void receivePendingConnections();
    QString serverName_;
    QLocalServer server_;
};

#endif // HAIKENANIME_ANILISTOAUTHCALLBACKRECEIVER_H
