#include "CoverDownloadCoordinator.h"
#include <QFile>
#include <QTimer>

CoverDownloadCoordinator::CoverDownloadCoordinator(ICoverDownloader &downloader, ICoverCacheRepository &cache,
                                                   ICoverFileStore &files, CoverSettings settings, QObject *parent)
    : QObject(parent), downloader_(downloader), cache_(cache), files_(files), settings_(settings)
{
    QString ignored;
    cache_.ReadAll(entries_, ignored);
}

QString CoverDownloadCoordinator::Key(const CoverRequest &r) const
{
    return QString::number(r.mediaId) + '|' + r.remoteUrl.toString() + '|' + CoverQualityName(r.quality);
}

void CoverDownloadCoordinator::Enqueue(const CoverRequest &input)
{
    if (input.mediaId <= 0 || !input.remoteUrl.isValid()) return;
    CoverRequest request = input;
    request.generation = generation_;
    const QString key = Key(request);
    if (queued_.contains(key) || active_.contains(key)) return;
    const auto current = entries_.value(request.mediaId);
    if (current.mediaId == request.mediaId && current.remoteUrl == request.remoteUrl.toString()
        && current.quality == request.quality && files_.Exists(current.relativePath)) {
        emit CoverAvailable(request.mediaId, files_.AbsolutePath(current.relativePath));
        emit CoverStateChanged(request.mediaId, CoverState::Available);
        return;
    }
    if (cooldowns_.value(key) > QDateTime::currentDateTimeUtc()) return;
    queued_.insert(key);
    (request.priority == CoverPriority::Visible ? visible_ : prefetch_).enqueue(request);
    emit CoverStateChanged(request.mediaId, CoverState::Queued);
}

void CoverDownloadCoordinator::RequestWindow(QList<CoverRequest> visible, QList<CoverRequest> prefetch)
{
    QSet<QString> retained;
    for (auto &r : visible) { r.priority = CoverPriority::Visible; retained.insert(Key(r)); }
    for (auto &r : prefetch) { r.priority = CoverPriority::Prefetch; retained.insert(Key(r)); }
    auto prune = [&](QQueue<CoverRequest> &queue) {
        QQueue<CoverRequest> next;
        while (!queue.isEmpty()) { auto r = queue.dequeue(); if (retained.contains(Key(r))) next.enqueue(r); else queued_.remove(Key(r)); }
        queue = next;
    };
    prune(visible_); prune(prefetch_);
    for (const auto &r : visible) Enqueue(r);
    for (const auto &r : prefetch) Enqueue(r);
    Pump();
}

void CoverDownloadCoordinator::Pump()
{
    while (active_.size() < settings_.maxConcurrentDownloads && (!visible_.isEmpty() || !prefetch_.isEmpty())) {
        CoverRequest request = !visible_.isEmpty() ? visible_.dequeue() : prefetch_.dequeue();
        const QString key = Key(request);
        queued_.remove(key);
        emit CoverStateChanged(request.mediaId, CoverState::Downloading);
        const int attempt = attempts_.value(key, 0);
        quint64 id = downloader_.Start(request, [this, key, generation = generation_, attempt](CoverDownloadResult result) {
            Complete(key, generation, attempt, std::move(result));
        });
        active_.insert(key, {id, request, 0});
    }
}

void CoverDownloadCoordinator::Complete(QString key, quint64 generation, int attempt, CoverDownloadResult result)
{
    active_.remove(key);
    if (generation != generation_) { QFile::remove(result.temporaryPath); Pump(); return; }
    const bool temporary = result.failure == CoverFailureCategory::Transport || result.failure == CoverFailureCategory::HttpTemporary;
    if (!result.succeeded && temporary && attempt < settings_.maxRetries) {
        const int delay = qMax(settings_.retryDelayMs, result.retryAfterMs);
        attempts_[key] = attempt + 1;
        QTimer::singleShot(delay, this, [this, key, request = result.request, generation] {
            if (generation != generation_ || active_.contains(key) || queued_.contains(key)) return;
            queued_.insert(key);
            (request.priority == CoverPriority::Visible ? visible_ : prefetch_).prepend(request);
            emit CoverStateChanged(request.mediaId, CoverState::Queued);
            Pump();
        });
        Pump();
        return;
    }
    if (!result.succeeded) {
        attempts_.remove(key);
        cooldowns_[key] = QDateTime::currentDateTimeUtc().addMSecs(settings_.failureCooldownMs);
        emit CoverStateChanged(result.request.mediaId, CoverState::Failed);
        Pump();
        return;
    }
    PublishedCover published;
    QString error;
    if (!files_.Publish(result.request.mediaId, result.request.remoteUrl.toString(), result.temporaryPath,
                        result.mimeType, published, error)) {
        emit CoverStateChanged(result.request.mediaId, CoverState::Failed); Pump(); return;
    }
    CoverCacheEntry old = entries_.value(result.request.mediaId);
    CoverCacheEntry entry{result.request.mediaId, result.request.remoteUrl.toString(), result.request.quality,
                          published.relativePath, published.mimeType, published.byteSize, result.etag,
                          result.lastModified, QDateTime::currentDateTimeUtc()};
    if (!cache_.Upsert(entry, error)) {
        files_.Remove(published.relativePath, error);
        emit CoverStateChanged(result.request.mediaId, CoverState::Failed); Pump(); return;
    }
    entries_[entry.mediaId] = entry;
    attempts_.remove(key);
    if (old.mediaId && old.relativePath != entry.relativePath) files_.Remove(old.relativePath, error);
    QFile::remove(result.temporaryPath);
    emit CoverAvailable(entry.mediaId, files_.AbsolutePath(entry.relativePath));
    emit CoverStateChanged(entry.mediaId, CoverState::Available);
    Pump();
}

void CoverDownloadCoordinator::ReportMissingFile(int mediaId)
{
    QString error;
    entries_.remove(mediaId);
    cache_.Remove(mediaId, error);
    emit CoverStateChanged(mediaId, CoverState::Missing);
}

void CoverDownloadCoordinator::Clear()
{
    ++generation_;
    for (const auto &active : std::as_const(active_)) downloader_.Cancel(active.id);
    active_.clear(); visible_.clear(); prefetch_.clear(); queued_.clear(); entries_.clear(); cooldowns_.clear(); attempts_.clear();
    QString error;
    cache_.Clear(error);
    files_.Clear(error);
    emit ClearCompleted();
}
