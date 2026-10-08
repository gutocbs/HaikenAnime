#ifndef HAIKENANIME_COVERDOWNLOADCOORDINATOR_H
#define HAIKENANIME_COVERDOWNLOADCOORDINATOR_H

#include "CoverSettings.h"
#include "ICoverCacheRepository.h"
#include "ICoverDownloader.h"
#include "ICoverFileStore.h"
#include <QHash>
#include <QObject>
#include <QQueue>

class AsyncLogger;

struct CoverCacheCleanupFailure final {
    QString component;
    QString error;
};

struct CoverCacheCleanupResult final {
    int removedFiles = 0;
    int removedEntries = 0;
    QList<CoverCacheCleanupFailure> failures;

    [[nodiscard]] bool Succeeded() const { return failures.isEmpty(); }
};

class CoverDownloadCoordinator final : public QObject {
    Q_OBJECT
public:
    CoverDownloadCoordinator(ICoverDownloader &downloader, ICoverCacheRepository &cache,
                             ICoverFileStore &files, CoverSettings settings, QObject *parent = nullptr);
    void setLogger(AsyncLogger *logger);
    void RequestWindow(QList<CoverRequest> visible, QList<CoverRequest> prefetch);
    void ReportMissingFile(int mediaId);
    CoverCacheCleanupResult Clear();

signals:
    void CoverAvailable(int mediaId, QString absolutePath);
    void CoverStateChanged(int mediaId, CoverState state);
    void ClearCompleted();

private:
    struct Active { quint64 id; CoverRequest request; int attempt; };
    QString Key(const CoverRequest &request) const;
    void Enqueue(const CoverRequest &request);
    void Pump();
    void Complete(QString key, quint64 generation, int attempt, CoverDownloadResult result);

    ICoverDownloader &downloader_;
    ICoverCacheRepository &cache_;
    ICoverFileStore &files_;
    CoverSettings settings_;
    QQueue<CoverRequest> visible_;
    QQueue<CoverRequest> prefetch_;
    QHash<QString, Active> active_;
    QSet<QString> queued_;
    QHash<int, CoverCacheEntry> entries_;
    QHash<QString, QDateTime> cooldowns_;
    QHash<QString, int> attempts_;
    quint64 generation_ = 1;
    bool cleanupInProgress_ = false;
    AsyncLogger *logger_ = nullptr;
};
#endif
