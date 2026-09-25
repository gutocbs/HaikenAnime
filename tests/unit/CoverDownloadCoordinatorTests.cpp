#include "application/covers/CoverDownloadCoordinator.h"
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class FakeDownloader final : public ICoverDownloader {
public:
    quint64 Start(const CoverRequest &r, Completion c) override { order << r.mediaId; completions[r.mediaId] = std::move(c); return ++id; }
    void Cancel(quint64 requestId) override { cancelled.insert(requestId); }
    QList<int> order; QHash<int, Completion> completions; QSet<quint64> cancelled; quint64 id = 0;
};
class FakeCache final : public ICoverCacheRepository {
public:
    bool ReadAll(QHash<int,CoverCacheEntry>& out, QString&) override { out=entries; return true; }
    bool Upsert(const CoverCacheEntry& e, QString& error) override {
        if (failUpsert) { error = "cache failure"; return false; }
        entries[e.mediaId]=e; return true;
    }
    bool Remove(int id, QString&) override { entries.remove(id); return true; }
    bool Clear(QString&) override { entries.clear(); return true; }
    QHash<int,CoverCacheEntry> entries;
    bool failUpsert = false;
};
class FakeFiles final : public ICoverFileStore {
public:
    bool Exists(const QString& p) const override { return existing.contains(p); }
    bool Publish(int id,const QString&,const QString&,const QString& mime,PublishedCover& out,QString& error) override {
        if (failPublish) { error = "publish failure"; return false; }
        published.insert(id); out={QString::number(id)+".png",mime,10}; existing.insert(out.relativePath); return true;
    }
    bool Remove(const QString& p,QString&) override { existing.remove(p); removed << p; return true; }
    bool Clear(QString&) override { existing.clear(); published.clear(); return true; }
    bool RemoveOrphans(const QSet<QString>&,int,int&,QString&) override{return true;}
    QString AbsolutePath(const QString& p) const override{return "/covers/"+p;}
    QSet<QString> existing; QSet<int> published; QStringList removed;
    bool failPublish = false;
};

class CoverDownloadCoordinatorTests : public QObject {
    Q_OBJECT
private slots:
    void boundsConcurrencyPrioritizesAndDeduplicates();
    void discardsCompletionAfterClear();
    void replacesOldFileOnlyAfterPersistence();
    void removesTemporaryFileWhenPublicationFails();
    void removesTemporaryFileWhenPersistenceFails();
};
static CoverRequest Req(int id, CoverPriority p=CoverPriority::Prefetch) { return {id,QUrl("https://example/"+QString::number(id)+".png"),CoverQuality::Medium,p,0}; }
static CoverDownloadResult Success(CoverRequest r) { CoverDownloadResult x; x.request=r; x.succeeded=true; x.temporaryPath="temp"; x.mimeType="image/png"; return x; }

void CoverDownloadCoordinatorTests::boundsConcurrencyPrioritizesAndDeduplicates()
{
    FakeDownloader d; FakeCache c; FakeFiles f; CoverSettings s; s.maxConcurrentDownloads=3; s.maxRetries=0;
    CoverDownloadCoordinator coordinator(d,c,f,s);
    coordinator.RequestWindow({Req(1),Req(2),Req(1)}, {Req(3),Req(4)});
    QCOMPARE(d.order, QList<int>({1,2,3}));
    d.completions.take(1)(Success(Req(1)));
    QCOMPARE(d.order, QList<int>({1,2,3,4}));
}
void CoverDownloadCoordinatorTests::discardsCompletionAfterClear()
{
    FakeDownloader d; FakeCache c; FakeFiles f; CoverSettings s; s.maxRetries=0;
    CoverDownloadCoordinator coordinator(d,c,f,s); auto request=Req(8);
    coordinator.RequestWindow({request}, {}); auto completion=d.completions.take(8);
    coordinator.Clear(); completion(Success(request));
    QVERIFY(c.entries.isEmpty()); QVERIFY(f.published.isEmpty());
}
void CoverDownloadCoordinatorTests::replacesOldFileOnlyAfterPersistence()
{
    FakeDownloader d; FakeCache c; FakeFiles f; CoverSettings s; s.maxRetries=0;
    c.entries[9]={9,"https://old",CoverQuality::Medium,"old.png","image/png",1}; f.existing.insert("old.png");
    CoverDownloadCoordinator coordinator(d,c,f,s); auto request=Req(9);
    coordinator.RequestWindow({request}, {}); d.completions.take(9)(Success(request));
    QCOMPARE(c.entries[9].relativePath, QString("9.png")); QVERIFY(f.removed.contains("old.png"));
}

void CoverDownloadCoordinatorTests::removesTemporaryFileWhenPublicationFails()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto temporaryPath = directory.filePath("download.tmp");
    QFile temporary(temporaryPath);
    QVERIFY(temporary.open(QIODevice::WriteOnly));
    temporary.write("image");
    temporary.close();

    FakeDownloader downloader;
    FakeCache cache;
    FakeFiles files;
    files.failPublish = true;
    CoverSettings settings;
    settings.maxRetries = 0;
    CoverDownloadCoordinator coordinator(downloader, cache, files, settings);
    const auto request = Req(10);
    coordinator.RequestWindow({request}, {});
    auto result = Success(request);
    result.temporaryPath = temporaryPath;
    downloader.completions.take(10)(result);

    QVERIFY(!QFileInfo::exists(temporaryPath));
}

void CoverDownloadCoordinatorTests::removesTemporaryFileWhenPersistenceFails()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto temporaryPath = directory.filePath("download.tmp");
    QFile temporary(temporaryPath);
    QVERIFY(temporary.open(QIODevice::WriteOnly));
    temporary.write("image");
    temporary.close();

    FakeDownloader downloader;
    FakeCache cache;
    cache.failUpsert = true;
    FakeFiles files;
    CoverSettings settings;
    settings.maxRetries = 0;
    CoverDownloadCoordinator coordinator(downloader, cache, files, settings);
    const auto request = Req(11);
    coordinator.RequestWindow({request}, {});
    auto result = Success(request);
    result.temporaryPath = temporaryPath;
    downloader.completions.take(11)(result);

    QVERIFY(!QFileInfo::exists(temporaryPath));
}
QTEST_MAIN(CoverDownloadCoordinatorTests)
#include "CoverDownloadCoordinatorTests.moc"
