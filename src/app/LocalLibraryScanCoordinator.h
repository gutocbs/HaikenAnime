#ifndef HAIKENANIME_LOCALLIBRARYSCANCOORDINATOR_H
#define HAIKENANIME_LOCALLIBRARYSCANCOORDINATOR_H

#include <QObject>
#include <atomic>
#include <functional>
#include <memory>

#include "../application/library/ILocalFileRepository.h"
#include "../application/library/ILocalLibraryScanner.h"

class QThread;

// Called on its QObject thread. Factories run on the worker and must return
// owning products: any enumerator or database connection must live with them.
class LocalLibraryScanCoordinator final : public QObject {
    Q_OBJECT
public:
    using ScannerFactory = std::function<std::unique_ptr<ILocalLibraryScanner>(QString &error)>;
    using RepositoryFactory = std::function<std::unique_ptr<ILocalFileRepository>(QString &error)>;

    LocalLibraryScanCoordinator(ScannerFactory scannerFactory, RepositoryFactory repositoryFactory,
                                QObject *parent = nullptr);
    ~LocalLibraryScanCoordinator() override;
    bool start(const LocalLibraryScanRequest &request);
    void shutdown();
signals:
    void started(QString rootPath);
    void progressChanged(qsizetype candidateFiles);
    void completed(qsizetype candidateFiles);
    void failed(QString error);
private:
    void execute(const LocalLibraryScanRequest &request);
    void releaseThread();
    ScannerFactory scannerFactory_;
    RepositoryFactory repositoryFactory_;
    QThread *thread_ = nullptr;
    std::atomic_bool stopRequested_ = false;
    bool executionActive_ = false;
    bool stopping_ = false;
};

#endif
