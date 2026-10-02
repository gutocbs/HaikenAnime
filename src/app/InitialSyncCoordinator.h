#ifndef HAIKENANIME_INITIALSYNCCOORDINATOR_H
#define HAIKENANIME_INITIALSYNCCOORDINATOR_H

#include <QObject>

#include <functional>

class QThread;
class QTimer;

class AsyncLogger;

class InitialSyncCoordinator final : public QObject {
    Q_OBJECT
public:
    using SyncOperation = std::function<bool(QString &)>;

    explicit InitialSyncCoordinator(QString databasePath, QString fixturePath,
                                    QString upsertQueryPath, QString readQueryPath,
                                    QString readActiveMediaIdsQueryPath,
                                    QString markSourceRemovedQueryPath,
                                    int syncTimeoutMs = 60000,
                                    int syncIntervalMs = 3600000,
                                    QObject *parent = nullptr);
    explicit InitialSyncCoordinator(SyncOperation operation,
                                    int syncIntervalMs = 3600000,
                                    QObject *parent = nullptr);
    ~InitialSyncCoordinator() override;
    void setLogger(AsyncLogger *logger);
    void configureAutomaticSynchronization(bool enabled, int intervalMs);
    [[nodiscard]] bool automaticSynchronizationEnabled() const;
    [[nodiscard]] int synchronizationIntervalMs() const;
public slots:
    void start();
    void shutdown();
signals:
    void started();
    void completed();
    void failed(const QString &error);
private:
    bool performSynchronization(QString &error) const;
    void releaseFinishedThread();

    QString databasePath_;
    QString fixturePath_;
    QString upsertQueryPath_;
    QString readQueryPath_;
    QString readActiveMediaIdsQueryPath_;
    QString markSourceRemovedQueryPath_;
    int syncTimeoutMs_;
    int syncIntervalMs_;
    QTimer *scheduler_ = nullptr;
    AsyncLogger *logger_ = nullptr;
    QThread *thread_ = nullptr;
    SyncOperation operation_;
    bool executionActive_ = false;
    bool stopping_ = false;
    bool automaticSynchronizationEnabled_ = true;
};

#endif
