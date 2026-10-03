#ifndef HAIKENANIME_LOCALLIBRARYRECOGNITIONCOORDINATOR_H
#define HAIKENANIME_LOCALLIBRARYRECOGNITIONCOORDINATOR_H

#include <QObject>
#include <functional>
#include <memory>
#include <atomic>
#include <QThread>

#include "../application/library/ILocalFileRepository.h"
#include "../application/library/ILocalFileRecognizer.h"
#include "../application/library/LocalMediaResolver.h"

class AsyncLogger;

class LocalLibraryRecognitionCoordinator final : public QObject {
    Q_OBJECT
public:
    using RepositoryFactory = std::function<std::unique_ptr<ILocalFileRepository>(QString &error)>;
    LocalLibraryRecognitionCoordinator(RepositoryFactory factory, QObject *parent = nullptr);
    ~LocalLibraryRecognitionCoordinator() override;
    void setLogger(AsyncLogger *logger);
    bool start(const QString &rootPath);
    void shutdown();
signals:
    void batchPersisted();
    void completed(qsizetype processed, qsizetype associated, qsizetype unrecognized, qsizetype ambiguous);
    void failed(QString error);
private:
    RepositoryFactory factory_;
    QThread *thread_ = nullptr;
    std::atomic_bool stopping_ = false;
    bool active_ = false;
    AsyncLogger *logger_ = nullptr;
};

#endif
