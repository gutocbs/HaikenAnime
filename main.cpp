#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QThread>
#include <QList>
#include <QTextStream>

// Temporary composition used to validate the synchronization worker.
#include "src/application/anilist/AniListSyncWorker.h"
#include "src/infrastructure/anilist/FileAniListDataSource.h"

class TestMediaRepository final : public IMediaRepository {
public:
    bool upsert(const QList<Media> &media, QString &error) override {
        Q_UNUSED(error)
        storedMedia.append(media);
        return true;
    }

    QList<Media> storedMedia;
};

int RunSynchronizationTest() {
    const auto fixturePath = QDir(QCoreApplication::applicationDirPath())
                                 .filePath(QStringLiteral("../tests/fixtures/media-library.json"));
    FileAniListDataSource dataSource(QDir::cleanPath(fixturePath));
    TestMediaRepository repository;
    AniListSyncService service(dataSource, repository);
    AniListSyncWorker worker(service);
    QThread thread;
    worker.moveToThread(&thread);

    const AniListSyncFilter filter;
    QObject::connect(&thread, &QThread::started, &worker, [&worker, filter]() { worker.Run(filter); },
                     Qt::DirectConnection);
    QObject::connect(&worker, &AniListSyncWorker::Completed, &thread, &QThread::quit,
                     Qt::DirectConnection);
    QObject::connect(&worker, &AniListSyncWorker::Failed, &thread, [&thread](const QString &message) {
        qCritical() << message;
        thread.quit();
    }, Qt::DirectConnection);

    thread.start();
    thread.wait();

    QTextStream output(stdout);
    output << "Media saved by synchronization: " << repository.storedMedia.size() << '\n';
    for (const auto &media : repository.storedMedia) {
        output << QStringLiteral("- id=%1, name=%2, englishName=%3, type=%4, status=%5\n")
                      .arg(media.Id)
                      .arg(media.Name)
                      .arg(media.EnglishName)
                      .arg(static_cast<int>(media.Type))
                      .arg(static_cast<int>(media.Status));
    }
    output.flush();
    return 0;
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    return RunSynchronizationTest();
}
