#ifndef HAIKENANIME_ANILISTOAUTHCALLBACKRECEIVER_H
#define HAIKENANIME_ANILISTOAUTHCALLBACKRECEIVER_H

#include <QObject>
#include <QLocalServer>
#include <QUrl>

#include <optional>
#include <functional>

/** Routes an AniList custom-scheme callback to the running application instance. */
class AniListOAuthCallbackReceiver final : public QObject {
    Q_OBJECT
public:
    enum class StartResult { Listening, Forwarded, Unavailable };
    using AuditLogger = std::function<void(const QString &)>;

    explicit AniListOAuthCallbackReceiver(QString serverName, QObject *parent = nullptr);
    ~AniListOAuthCallbackReceiver() override;
    void setAuditLogger(AuditLogger logger);

    [[nodiscard]] static std::optional<QUrl> callbackFromArguments(const QStringList &arguments);
    [[nodiscard]] StartResult start(const QStringList &arguments, QString &error);

signals:
    void callbackReceived(QUrl callback);

private:
    void audit(const QString &event) const;
    void receivePendingConnections();
    QString serverName_;
    AuditLogger auditLogger_;
    QLocalServer server_;
};

#endif // HAIKENANIME_ANILISTOAUTHCALLBACKRECEIVER_H
