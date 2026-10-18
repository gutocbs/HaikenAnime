#include "LocalLibraryRecognitionCoordinator.h"

#include "../infrastructure/library/AnitomyLocalFileRecognizer.h"
#include "../infrastructure/logging/AsyncLogger.h"
#include <QMetaObject>
#include <QThread>
#include <utility>

namespace {
constexpr qsizetype BatchSize = 200;

QString StateName(LocalRecognitionState state) {
    switch (state) {
    case LocalRecognitionState::Recognized: return QStringLiteral("recognized");
    case LocalRecognitionState::Associated: return QStringLiteral("associated");
    case LocalRecognitionState::Ambiguous: return QStringLiteral("ambiguous");
    case LocalRecognitionState::Unsupported: return QStringLiteral("unsupported");
    case LocalRecognitionState::Unprocessed: return QStringLiteral("unprocessed");
    case LocalRecognitionState::Unrecognized: return QStringLiteral("unrecognized");
    }
    return QStringLiteral("unrecognized");
}

struct RunCounters final {
    qsizetype processed = 0;
    qsizetype associated = 0;
    qsizetype unrecognized = 0;
    qsizetype ambiguous = 0;
};

QString AbsolutePathOf(const LocalFileRecognitionRecord &record) {
    if (record.relativePath.isEmpty()) return {};
    return record.rootPath + QLatin1Char('/') + record.relativePath;
}
}

LocalLibraryRecognitionCoordinator::LocalLibraryRecognitionCoordinator(RepositoryFactory factory, QObject *parent)
    : QObject(parent), factory_(std::move(factory)) {}

LocalLibraryRecognitionCoordinator::~LocalLibraryRecognitionCoordinator() { shutdown(); }

void LocalLibraryRecognitionCoordinator::setLogger(AsyncLogger *logger) {
    logger_ = logger;
}

bool LocalLibraryRecognitionCoordinator::startForPending(const QString &rootPath) {
    return submit({false, rootPath});
}

bool LocalLibraryRecognitionCoordinator::startForFolder(const QString &folderPath) {
    return submit({true, folderPath});
}

bool LocalLibraryRecognitionCoordinator::submit(Job job) {
    if (stopping_ || job.path.trimmed().isEmpty()) return false;
    if (active_) {
        // At most one follow-up run is pending; repeated requests for the same target coalesce into it.
        if (!queued_.contains(job)) queued_.append(std::move(job));
        return true;
    }
    launch({std::move(job)});
    return true;
}

void LocalLibraryRecognitionCoordinator::launch(QList<Job> jobs) {
    active_ = true;
    thread_ = QThread::create([this, jobs = std::move(jobs)] {
        QString error;
        RunCounters totals;
        bool cancelled = false;
        auto repository = factory_ ? factory_(error) : nullptr;
        if (!repository) {
            if (error.isEmpty()) error = QStringLiteral("Cannot create local recognition repository.");
        } else {
            QList<Media> catalog;
            if (repository->readRecognitionCatalog(catalog, error)) {
                AnitomyLocalFileRecognizer recognizer;
                LocalMediaResolver resolver;
                // Normalizing the catalog once keeps each file at one normalization plus one lookup.
                const LocalMediaCatalogIndex catalogIndex(catalog);
                for (const auto &job : jobs) {
                    if (stopping_.load()) { cancelled = true; break; }
                    const auto target = job.isFolder ? QStringLiteral("folder=%1").arg(job.path)
                                                     : QStringLiteral("pending root=%1").arg(job.path);
                    if (logger_) logger_->info(LogCategory::LocalLibrary,
                                               QStringLiteral("Local library recognition started: %1").arg(target));
                    QList<LocalFileRecognitionRecord> records;
                    const bool read = job.isFolder ? repository->readFilesUnderFolder(job.path, records, error)
                                                   : repository->readPendingRecognition(job.path, records, error);
                    if (!read) break;
                    RunCounters counters;
                    QList<LocalFileRecognitionRecord> updates;
                    for (auto record : records) {
                        if (stopping_.load()) { cancelled = true; break; }
                        const auto recognition = recognizer.recognize(record.fileName);
                        const auto match = resolver.resolve(recognition, catalogIndex, AbsolutePathOf(record));
                        record.recognitionState = StateName(match.state);
                        record.extractedTitle = recognition.extractedTitle;
                        record.mediaKind = recognition.mediaKind == LocalMediaKind::Anime ? QStringLiteral("anime") : QStringLiteral("unsupported");
                        record.season = recognition.season;
                        record.episode = recognition.episode;
                        record.mediaId = match.mediaId > 0 ? std::optional<int>(match.mediaId) : std::nullopt;
                        record.diagnostic = match.diagnostic.isEmpty() ? recognition.diagnostic : match.diagnostic;
                        updates.append(record); ++counters.processed;
                        if (record.recognitionState == QStringLiteral("associated")) ++counters.associated;
                        else if (record.recognitionState == QStringLiteral("ambiguous")) ++counters.ambiguous;
                        else ++counters.unrecognized;
                        if (updates.size() == BatchSize) {
                            if (!repository->saveRecognitionBatch(updates, error)) break;
                            QMetaObject::invokeMethod(this, [this] { emit batchPersisted(); }, Qt::QueuedConnection);
                            updates.clear();
                        }
                    }
                    // On cancellation the batch already processed is still persisted before stopping.
                    if (error.isEmpty() && !updates.isEmpty()) {
                        if (repository->saveRecognitionBatch(updates, error) && !cancelled) {
                            QMetaObject::invokeMethod(this, [this] { emit batchPersisted(); }, Qt::QueuedConnection);
                        }
                    }
                    totals.processed += counters.processed;
                    totals.associated += counters.associated;
                    totals.unrecognized += counters.unrecognized;
                    totals.ambiguous += counters.ambiguous;
                    if (logger_ && error.isEmpty()) {
                        logger_->info(LogCategory::LocalLibrary,
                                      QStringLiteral("Local library recognition completed: %1, processed=%2, "
                                                     "associated=%3, unrecognized=%4, ambiguous=%5, cancelled=%6")
                                          .arg(target).arg(counters.processed).arg(counters.associated)
                                          .arg(counters.unrecognized).arg(counters.ambiguous).arg(cancelled ? 1 : 0));
                    }
                    if (!error.isEmpty() || cancelled) break;
                }
            }
        }
        if (!error.isEmpty() && logger_) {
            logger_->error(LogCategory::LocalLibrary,
                           QStringLiteral("Local library recognition failed: processed=%1, error=%2")
                               .arg(totals.processed).arg(error));
        }
        QMetaObject::invokeMethod(this, [this, error, totals, cancelled] {
            finishRun(cancelled, error, totals.processed, totals.associated, totals.unrecognized, totals.ambiguous);
        }, Qt::QueuedConnection);
    });
    thread_->start(QThread::LowPriority);
}

void LocalLibraryRecognitionCoordinator::finishRun(bool cancelled, const QString &error, qsizetype processed,
                                                   qsizetype associated, qsizetype unrecognized,
                                                   qsizetype ambiguous) {
    if (thread_) { thread_->wait(); delete thread_; thread_ = nullptr; }
    active_ = false;
    if (cancelled) { queued_.clear(); return; }
    if (error.isEmpty()) emit completed(processed, associated, unrecognized, ambiguous);
    else emit failed(error);
    if (!queued_.isEmpty() && !stopping_) launch(std::exchange(queued_, {}));
}

void LocalLibraryRecognitionCoordinator::shutdown() {
    stopping_ = true;
    queued_.clear();
    if (thread_) { thread_->wait(); delete thread_; thread_ = nullptr; }
    active_ = false;
}
