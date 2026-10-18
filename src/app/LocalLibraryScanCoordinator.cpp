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
    return startAll({request});
}

bool LocalLibraryScanCoordinator::startAll(const QList<LocalLibraryScanRequest> &requests) {
    if (stopping_ || executionActive_ || requests.isEmpty()) return false;
    releaseThread();
    executionActive_ = true;
    stopRequested_ = false;
    thread_ = QThread::create([this, requests] { execute(requests); });
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

void LocalLibraryScanCoordinator::execute(const QList<LocalLibraryScanRequest> &requests) {
    qsizetype totalCandidates = 0;
    QStringList errors;
    QList<std::pair<QString, qsizetype>> succeededRoots;
    for (const auto &request : requests) {
        if (stopRequested_.load()) {
            errors.append(QStringLiteral("Local library scan interrupted."));
            break;
        }
        qsizetype candidateFiles = 0;
        QString error;
        // One unreachable folder (an unplugged drive) must not hide the others' files.
        if (executeRoot(request, totalCandidates, candidateFiles, error)) {
            totalCandidates += candidateFiles;
            succeededRoots.append({request.rootPath, candidateFiles});
        } else {
            errors.append(requests.size() > 1 ? QStringLiteral("%1: %2").arg(request.rootPath, error) : error);
        }
    }
    QMetaObject::invokeMethod(this, [this, totalCandidates, succeededRoots,
                                     error = errors.join(QLatin1Char('\n'))] {
        // Join before accepting another start, even if the queued callback runs
        // just before QThread has finished unwinding its worker function.
        releaseThread();
        executionActive_ = false;
        if (error.isEmpty()) emit completed(totalCandidates);
        // Successful roots are still recognized when another root failed.
        for (const auto &[rootPath, candidateFiles] : succeededRoots) emit succeeded(rootPath, candidateFiles);
        if (!error.isEmpty()) emit failed(error);
    }, Qt::QueuedConnection);
}

bool LocalLibraryScanCoordinator::executeRoot(const LocalLibraryScanRequest &request,
                                              const qsizetype candidateOffset,
                                              qsizetype &candidateFiles, QString &error) {
    QElapsedTimer elapsed;
    elapsed.start();
    QMetaObject::invokeMethod(this, [this, root = request.rootPath] { emit started(root); }, Qt::QueuedConnection);
    if (logger_) {
        logger_->info(LogCategory::LocalLibrary,
                      QStringLiteral("Local library scan started: root=%1").arg(request.rootPath));
    }

    qsizetype skippedFiles = 0;
    bool scanSucceeded = false;
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
                // Progress is cumulative across roots so the count never jumps backwards.
                const auto present = [this, &lastPresented, candidateOffset](qsizetype count) {
                    if (count == lastPresented) return;
                    lastPresented = count;
                    QMetaObject::invokeMethod(this, [this, total = candidateOffset + count] {
                        emit progressChanged(total);
                    }, Qt::QueuedConnection);
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
                scanSucceeded = repository->completeScan(scanId, candidateFiles, error);
                if (!scanSucceeded && error.isEmpty()) error = QStringLiteral("Cannot complete local library scan.");
            } else {
                error = !batchError.isEmpty() ? batchError : result.diagnostic;
                if (error.isEmpty()) error = interrupted ? QStringLiteral("Local library scan interrupted.")
                                                       : QStringLiteral("Local library scan incomplete.");
            }
            if (!scanSucceeded) {
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
        const auto outcome = scanSucceeded ? (candidateFiles == 0 ? QStringLiteral("zero-results")
                                                               : QStringLiteral("succeeded"))
                                       : (stopRequested_.load() ? QStringLiteral("interrupted")
                                                                : QStringLiteral("failed"));
        const auto message = QStringLiteral("Local library scan completed: outcome=%1, elapsedMs=%2, candidateFiles=%3%4, skippedFiles=%5")
                                 .arg(outcome)
                                 .arg(elapsed.elapsed())
                                 .arg(candidateFiles)
                                 .arg(scanSucceeded ? QString() : QStringLiteral(", error=%1").arg(error))
                                 .arg(skippedFiles);
        if (scanSucceeded && candidateFiles > 0) logger_->info(LogCategory::LocalLibrary, message);
        else if (scanSucceeded || stopRequested_.load()) logger_->warning(LogCategory::LocalLibrary, message);
        else logger_->error(LogCategory::LocalLibrary, message);
    }
    return scanSucceeded;
}
