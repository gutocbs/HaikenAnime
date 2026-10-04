#ifndef HAIKENANIME_ADAPTIVESYNCCOORDINATOR_H
#define HAIKENANIME_ADAPTIVESYNCCOORDINATOR_H

#include <QObject>

#include <functional>
#include <map>

#include "../application/scheduling/ISyncTaskExecutor.h"
#include "../application/scheduling/ISyncTaskStateRepository.h"
#include "../application/scheduling/SyncTaskPolicy.h"

class QTimer;

/** Schedules persisted synchronization tasks without owning network or SQLite execution objects. */
class AdaptiveSyncCoordinator final : public QObject {
    Q_OBJECT
public:
    using Clock = std::function<QDateTime()>;

    explicit AdaptiveSyncCoordinator(ISyncTaskStateRepository &stateRepository,
                                     ISyncTaskExecutor &executor,
                                     Clock clock = {},
                                     std::map<SyncTaskKind, SyncSchedulePolicy> policies = DefaultSyncTaskPolicies(),
                                     QObject *parent = nullptr);
    ~AdaptiveSyncCoordinator() override;

    [[nodiscard]] bool Start();
    void Stop();
    void RequestNow(SyncPartition partition);
    void NotifyLocalChange(SyncPartition partition);

public slots:
    void ProcessDueTasks();

signals:
    void TaskStarted(SyncPartition partition);
    void TaskCompleted(SyncPartition partition);
    void TaskFailed(SyncPartition partition, const QString &safeError);
    void SchedulingFailed(const QString &safeError);
    void Stopped();

private:
    void Request(SyncPartition partition, SyncTaskTrigger trigger);
    void StartTask(SyncTaskState state);
    void CompleteTask(SyncPartition partition, qint64 generation, SyncTaskExecutionResult result);
    void ScheduleWakeUp();
    bool Persist(const SyncTaskState &state);
    void AcknowledgeCancellation(SyncPartition partition, qint64 generation);
    void AcknowledgeShutdown();
    void FinishStopping();
    [[nodiscard]] QDateTime Now() const;
    [[nodiscard]] const SyncSchedulePolicy &PolicyFor(SyncTaskKind kind) const;

    ISyncTaskStateRepository &stateRepository_;
    ISyncTaskExecutor &executor_;
    Clock clock_;
    std::map<SyncTaskKind, SyncSchedulePolicy> policies_;
    std::map<SyncPartition, SyncTaskState> states_;
    std::map<SyncPartition, qint64> activeGenerations_;
    QTimer *wakeUpTimer_ = nullptr;
    bool started_ = false;
    bool stopped_ = false;
    bool stopping_ = false;
    bool shutdownAcknowledged_ = false;
};

#endif // HAIKENANIME_ADAPTIVESYNCCOORDINATOR_H
