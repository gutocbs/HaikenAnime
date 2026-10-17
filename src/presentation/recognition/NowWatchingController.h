#ifndef HAIKENANIME_NOWWATCHINGCONTROLLER_H
#define HAIKENANIME_NOWWATCHINGCONTROLLER_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

#include <functional>

#include "../../application/media/IMediaReader.h"
#include "../../application/recognition/ActiveSessionRegistry.h"

/// "Assistindo agora": the sessions being recognized right now, one shown at a time. Anime data
/// comes from the local library; episode data is whatever the plugin already reported.
class NowWatchingController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool active READ active NOTIFY changed)
    Q_PROPERTY(QVariantList sessions READ sessions NOTIFY changed)
    Q_PROPERTY(QString selectedSessionKey READ selectedSessionKey NOTIFY changed)
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)

public:
    NowWatchingController(ActiveSessionRegistry &registry, IMediaReader &media,
                          std::function<void(const QString &)> warning = {}, QObject *parent = nullptr);
    ~NowWatchingController() override;

    [[nodiscard]] bool active() const { return !sessions_.isEmpty(); }
    /// [{ key, label }] in start order, for the session selector.
    [[nodiscard]] QVariantList sessions() const { return sessions_; }
    [[nodiscard]] QString selectedSessionKey() const { return selectedKey_; }
    /// The selected session; optional values are absent keys, never empty strings.
    [[nodiscard]] QVariantMap current() const { return current_; }

    void setPreferredTitleKey(const QString &key);
    Q_INVOKABLE void SelectSession(const QString &key);
    /// Rebuilds sessions and the selected card from the registry and the library.
    void Refresh();

signals:
    void changed();

private:
    [[nodiscard]] QVariantMap buildCurrent(const ActiveSession &session, const QHash<int, Media> &library) const;

    ActiveSessionRegistry &registry_;
    IMediaReader &media_;
    std::function<void(const QString &)> warning_;
    QString preferredTitleKey_;
    QVariantList sessions_;
    QString selectedKey_;
    QVariantMap current_;
};

#endif // HAIKENANIME_NOWWATCHINGCONTROLLER_H
