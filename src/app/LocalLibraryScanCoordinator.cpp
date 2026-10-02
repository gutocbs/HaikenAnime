#include "LocalLibraryScanCoordinator.h"

#include "../infrastructure/logging/AsyncLogger.h"

#include <QElapsedTimer>
#include <QMetaObject>
#include <QThread>

#include <utility>

LocalLibraryScanCoordinator::LocalLibraryScanCoordinator(ScannerFactory scannerFactory,
    RepositoryFactory repositoryFactory, QObject *parent)
    : QObject(parent), scannerFactory_(std::move(scannerFactory)), repositoryFactory_(std::move(repositoryFactory)) {}
LocalLibraryScanCoordinator::~LocalLibraryScanCoordinator() { shutdown(); }

void LocalLibraryScanCoordinator::setLogger(AsyncLogger *logger) {
    logger_ = logger;
}

bool LocalLibraryScanCoordinator::start(const LocalLibraryScanRequest &request) {
    if (stopping_ || executionActive_) return false;
    releaseThread();
    executionActive_ = true;
    stopRequested_ = false;
    if (logger_) {
        logger_->info(LogCategory::LocalLibrary,
                      QStringLiteral("Local library scan started: root=%1").arg(request.rootPath));
    }
    thread_ = QThread::create([this, request] { execute(request); });
    thread_->start(QThread::LowPriority);
    return true;
}

void LocalLibraryScanCoordinator::shutdown() {
    stopping_ = true;
    stopRequested_ = true;
    releaseThread();
    executionActive_ = false;
}

void LocalLibraryScanCoordinator::releaseThread() {
    if (!thread_) return;
    thread_->wait();
    delete thread_;
    thread_ = nullptr;
}

void LocalLibraryScanCoordinator::execute(const LocalLibraryScanRequest &request) {
    QElapsedTimer elapsed;
    elapsed.start();
    QMetaObject::invokeMethod(this, [this, root = request.rootPath] { emit started(root); }, Qt::QueuedConnection);

    QString error;
    qsizetype candidateFiles = 0;
    qsizetype skippedFiles = 0;
    bool succeeded = false;
    // Resource scope ends before queuing the terminal signal. Factory products
    // are both constructed and destroyed on this worker, including connections.
    {
        auto repository = repositoryFactory_ ? repositoryFactory_(error) : nullptr;
        qint64 scanId = 0;
        if (!repository) {
            if (error.isEmpty()) error = QStringLiteral("Cannot create local library repository.");
        } else if (!repository->beginScan(request.rootPath, scanId, error)) {
            if (error.isEmpty()) error = QStringLiteral("Cannot begin local library scan.");
        } else {
            auto scanner = scannerFactory_ ? scannerFactory_(error) : nullptr;
            LocalLibraryScanResult result;
            QString batchError;
            if (scanner) {
                QElapsedTimer progressTimer;
                progressTimer.start();
                qsizetype lastPresented = -1;
                const auto present = [this, &lastPresented](qsizetype count) {
                    if (count == lastPresented) return;
                    lastPresented = count;
                    QMetaObject::invokeMethod(this, [this, count] { emit progressChanged(count); }, Qt::QueuedConnection);
                };
                result = scanner->scan(request,
                    [&](const QList<LocalFileObservation> &batch, QString &consumerError) {
                        if (!batchError.isEmpty()) { consumerError = batchError; return false; }
                        if (repository->upsertBatch(scanId, batch, consumerError)) return true;
                        batchError = consumerError.isEmpty() ? QStringLiteral("Local library batch write failed.") : consumerError;
                        consumerError = batchError;
                        return false;
                    },
                    [&](const LocalLibraryScanProgress &progress) {
                        // Scanner progress may arrive for each entry. Only one
                        // queued UI update per interval and a final snapshot escape.
                        if (progressTimer.elapsed() >= 100) {
                            present(progress.candidateFiles);
                            progressTimer.restart();
                        }
                    },
                    [this] { return stopRequested_.load(); });
                candidateFiles = result.candidateFiles;
                skippedFiles = result.skippedFiles;
                present(candidateFiles);
            } else {
                result.diagnostic = error.isEmpty() ? QStringLiteral("Cannot create local library scanner.") : error;
            }

            const bool interrupted = result.interrupted || stopRequested_.load();
            if (result.complete && !interrupted && batchError.isEmpty()) {
                succeeded = repository->completeScan(scanId, candidateFiles, error);
                if (!succeeded && error.isEmpty()) error = QStringLiteral("Cannot complete local library scan.");
            } else {
                error = !batchError.isEmpty() ? batchError : result.diagnostic;
                if (error.isEmpty()) error = interrupted ? QStringLiteral("Local library scan interrupted.")
                                                       : QStringLiteral("Local library scan incomplete.");
            }
            if (!succeeded) {
                QString recordingError;
                if (!repository->failScan(scanId, interrupted ? LibraryScanStatus::Interrupted : LibraryScanStatus::Failed,
                                          candidateFiles, error, recordingError)) {
                    if (recordingError.isEmpty()) recordingError = QStringLiteral("Cannot record local library scan failure.");
                    error += QStringLiteral("\n") + recordingError;
                }
            }
        }
    }
    if (logger_) {
        const auto outcome = succeeded ? (candidateFiles == 0 ? QStringLiteral("zero-results")
                                                               : QStringLiteral("succeeded"))
                                       : (stopRequested_.load() ? QStringLiteral("interrupted")
                                                                : QStringLiteral("failed"));
        const auto message = QStringLiteral("Local library scan completed: outcome=%1, elapsedMs=%2, candidateFiles=%3%4, skippedFiles=%5")
                                 .arg(outcome)
                                 .arg(elapsed.elapsed())
                                 .arg(candidateFiles)
                                 .arg(succeeded ? QString() : QStringLiteral(", error=%1").arg(error))
                                 .arg(skippedFiles);
        if (succeeded && candidateFiles > 0) logger_->info(LogCategory::LocalLibrary, message);
        else if (succeeded || stopRequested_.load()) logger_->warning(LogCategory::LocalLibrary, message);
        else logger_->error(LogCategory::LocalLibrary, message);
    }
    QMetaObject::invokeMethod(this, [this, succeeded, candidateFiles, error = std::move(error)] {
        // Join before accepting another start, even if the queued callback runs
        // just before QThread has finished unwinding its worker function.
        releaseThread();
        executionActive_ = false;
        if (succeeded) emit completed(candidateFiles);
        else emit failed(error);
    }, Qt::QueuedConnection);
}
