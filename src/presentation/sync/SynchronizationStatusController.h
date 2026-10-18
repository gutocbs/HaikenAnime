#ifndef HAIKENANIME_SYNCHRONIZATIONSTATUSCONTROLLER_H
#define HAIKENANIME_SYNCHRONIZATIONSTATUSCONTROLLER_H

#include <QObject>
#include <QSet>
#include <QString>
#include <QTimer>

#include "../../application/scheduling/SyncProgress.h"
#include "../../application/scheduling/SyncTaskTypes.h"

/**
 * Single owner of the AniList synchronization state shown to the user.
 *
 * Sync outcomes are isolated here so Home and Settings never display synchronization errors.
 * Lives on the GUI thread; runtime signals reach it through queued connections.
 */
class SynchronizationStatusController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY changed)
    Q_PROPERTY(QString outcome READ outcome NOTIFY changed)
    Q_PROPERTY(QString stageKey READ stageKey NOTIFY changed)
    Q_PROPERTY(QString stageLabel READ stageLabel NOTIFY changed)
    Q_PROPERTY(int stageIndex READ stageIndex NOTIFY changed)
    Q_PROPERTY(int stageCount READ stageCount NOTIFY changed)
    Q_PROPERTY(int processedItems READ processedItems NOTIFY changed)
    Q_PROPERTY(int totalItems READ totalItems NOTIFY changed)
    Q_PROPERTY(bool progressKnown READ progressKnown NOTIFY changed)
    Q_PROPERTY(int percent READ percent NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(bool visible READ visible NOTIFY changed)

public:
    explicit SynchronizationStatusController(int hideDelayMs = 4000, int settleDelayMs = 750,
                                             QObject *parent = nullptr);

    bool running() const;
    QString outcome() const;
    QString stageKey() const;
    QString stageLabel() const;
    int stageIndex() const;
    int stageCount() const;
    int processedItems() const;
    int totalItems() const;
    bool progressKnown() const;
    int percent() const;
    QString message() const;
    bool visible() const;

public slots:
    void HandleStarted(SyncPartition partition);
    void HandleProgress(const SyncProgress &progress);
    void HandleCompleted(SyncPartition partition);
    void HandleCancelled(SyncPartition partition);
    void HandleFailed(SyncPartition partition, const QString &safeError);
    void HandleInitializationFailed(const QString &safeError);

signals:
    void changed();

private:
    void beginRoundIfSettled();
    void settleIfIdle();
    void publishOutcome();
    void hide();
    void finishSilently();

    QTimer hideTimer_;
    QTimer settleTimer_;
    QSet<int> activePartitions_;
    bool roundFailed_ = false;
    bool roundHadOutcome_ = false;
    bool visible_ = false;
    bool roundActive_ = false;
    bool roundShown_ = false;
    QString outcome_ = QStringLiteral("idle");
    QString stageKey_;
    int stageIndex_ = 0;
    int stageCount_ = 0;
    int processedItems_ = 0;
    int totalItems_ = -1;
    QString message_;
    QString failureMessage_;
};

#endif // HAIKENANIME_SYNCHRONIZATIONSTATUSCONTROLLER_H
