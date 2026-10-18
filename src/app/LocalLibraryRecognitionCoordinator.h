#ifndef HAIKENANIME_LOCALLIBRARYRECOGNITIONCOORDINATOR_H
#define HAIKENANIME_LOCALLIBRARYRECOGNITIONCOORDINATOR_H

#include <QObject>
#include <QList>
#include <functional>
#include <memory>
#include <atomic>
#include <QThread>

#include "../application/library/ILocalFileRecognitionRepository.h"
#include "../application/library/ILocalFileRecognizer.h"
#include "../application/library/LocalMediaResolver.h"

class AsyncLogger;

class LocalLibraryRecognitionCoordinator final : public QObject {
    Q_OBJECT
public:
    using RepositoryFactory = std::function<std::unique_ptr<ILocalFileRecognitionRepository>(QString &error)>;
    LocalLibraryRecognitionCoordinator(RepositoryFactory factory, QObject *parent = nullptr);
    ~LocalLibraryRecognitionCoordinator() override;
    void setLogger(AsyncLogger *logger);
    // Recognizes the pending files (unprocessed, unrecognized or ambiguous) of the library root.
    bool startForPending(const QString &rootPath);
    // Recognizes every available file under the folder, associated ones included, and re-associates them.
    bool startForFolder(const QString &folderPath);
    void shutdown();
signals:
    void batchPersisted();
    void completed(qsizetype processed, qsizetype associated, qsizetype unrecognized, qsizetype ambiguous);
    void failed(QString error);
private:
    struct Job {
        bool isFolder = false;
        QString path;
        bool operator==(const Job &other) const { return isFolder == other.isFolder && path == other.path; }
    };
    // Runs now when idle; otherwise joins the single follow-up run, ignoring duplicates.
    bool submit(Job job);
    void launch(QList<Job> jobs);
    void finishRun(bool cancelled, const QString &error, qsizetype processed, qsizetype associated,
                   qsizetype unrecognized, qsizetype ambiguous);

    RepositoryFactory factory_;
    QThread *thread_ = nullptr;
    std::atomic_bool stopping_ = false;
    bool active_ = false;
    QList<Job> queued_;
    AsyncLogger *logger_ = nullptr;
};

#endif
