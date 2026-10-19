#ifndef HAIKENANIME_LOCALLIBRARYSCANCOORDINATOR_H
#define HAIKENANIME_LOCALLIBRARYSCANCOORDINATOR_H

#include <QObject>
#include <atomic>
#include <functional>
#include <memory>

#include "../application/library/ILocalFileScanRepository.h"
#include "../application/library/ILocalLibraryScanner.h"

class QThread;
class AsyncLogger;

// Called on its QObject thread. Factories run on the worker and must return
// owning products: any enumerator or database connection must live with them.
class LocalLibraryScanCoordinator final : public QObject {
    Q_OBJECT
public:
    using ScannerFactory = std::function<std::unique_ptr<ILocalLibraryScanner>(QString &error)>;
    using RepositoryFactory = std::function<std::unique_ptr<ILocalFileScanRepository>(QString &error)>;

    LocalLibraryScanCoordinator(ScannerFactory scannerFactory, RepositoryFactory repositoryFactory,
                                QObject *parent = nullptr);
    ~LocalLibraryScanCoordinator() override;
    // Non-owning: ApplicationComposition owns the logger and keeps it alive
    // until this coordinator has joined its worker during destruction.
    void setLogger(AsyncLogger *logger);
    bool start(const LocalLibraryScanRequest &request);
    // Scans each root in order on one worker. started fires as each root begins; after the
    // worker is joined, succeeded fires per successful root and a single completed (total
    // candidates) or failed (every root error) closes the whole run.
    bool startAll(const QList<LocalLibraryScanRequest> &requests);
    void shutdown();
signals:
    void started(QString rootPath);
    void progressChanged(qsizetype candidateFiles);
    void completed(qsizetype candidateFiles);
    void succeeded(QString rootPath, qsizetype candidateFiles);
    void failed(QString error);
private:
    void execute(const QList<LocalLibraryScanRequest> &requests);
    bool executeRoot(const LocalLibraryScanRequest &request, qsizetype candidateOffset,
                     qsizetype &candidateFiles, QString &error);
    void releaseThread();
    ScannerFactory scannerFactory_;
    RepositoryFactory repositoryFactory_;
    QThread *thread_ = nullptr;
    AsyncLogger *logger_ = nullptr;
    std::atomic_bool stopRequested_ = false;
    bool executionActive_ = false;
    bool stopping_ = false;
};

#endif
