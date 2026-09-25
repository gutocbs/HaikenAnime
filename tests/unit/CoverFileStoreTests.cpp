#include "infrastructure/covers/CoverFileStore.h"

#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QtTest>

class CoverFileStoreTests : public QObject {
    Q_OBJECT

private slots:
    void publishesValidImageDeterministically();
    void rejectsInvalidImagesAndPreservesExistingFile();
    void removesOnlyBoundedOrphans();
};

static QString CreatePng(const QString &path, const QSize &size = {64, 96})
{
    QImage image(size, QImage::Format_ARGB32);
    image.fill(Qt::magenta);
    return image.save(path, "PNG") ? path : QString{};
}

void CoverFileStoreTests::publishesValidImageDeterministically()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    CoverSettings settings;
    CoverFileStore store(directory.path() + "/covers", settings);
    const auto source = CreatePng(directory.path() + "/source.png");
    QVERIFY(!source.isEmpty());

    PublishedCover first;
    QString error;
    QVERIFY2(store.Publish(42, "https://cdn.example/cover.png", source, "image/png", first, error), qPrintable(error));
    QVERIFY(!first.relativePath.isEmpty());
    QVERIFY(QDir::isRelativePath(first.relativePath));
    QCOMPARE(first.mimeType, QString("image/png"));
    QVERIFY(first.byteSize > 0);
    QVERIFY(store.Exists(first.relativePath));

    PublishedCover second;
    QVERIFY2(store.Publish(42, "https://cdn.example/cover.png", source, "image/png", second, error), qPrintable(error));
    QCOMPARE(second.relativePath, first.relativePath);
    QCOMPARE(store.AbsolutePath(first.relativePath), QDir(directory.path() + "/covers").filePath(first.relativePath));
}

void CoverFileStoreTests::rejectsInvalidImagesAndPreservesExistingFile()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    CoverSettings settings;
    settings.maxResponseBytes = 1024 * 1024;
    settings.minDimension = 32;
    settings.maxDimension = 256;
    CoverFileStore store(directory.path() + "/covers", settings);
    const auto valid = CreatePng(directory.path() + "/valid.png");
    PublishedCover published;
    QString error;
    QVERIFY(store.Publish(7, "https://cdn.example/same.png", valid, "image/png", published, error));
    QFile existing(store.AbsolutePath(published.relativePath));
    QVERIFY(existing.open(QIODevice::ReadOnly));
    const QByteArray original = existing.readAll();
    existing.close();

    const QStringList invalidPaths = {
        directory.path() + "/empty.png",
        directory.path() + "/html.png",
        directory.path() + "/truncated.png",
        CreatePng(directory.path() + "/small.png", {2, 2}),
        CreatePng(directory.path() + "/huge.png", {300, 300})
    };
    QFile(invalidPaths[0]).open(QIODevice::WriteOnly);
    for (int index : {1, 2}) {
        QFile file(invalidPaths[index]);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(index == 1 ? "<html>error</html>" : QByteArray("\x89PNG\r\n", 6));
    }
    const auto oversized = directory.path() + "/oversized.png";
    QFile oversizedFile(oversized);
    QVERIFY(oversizedFile.open(QIODevice::WriteOnly));
    oversizedFile.write(QByteArray(settings.maxResponseBytes + 1, 'x'));
    oversizedFile.close();

    QStringList candidates = invalidPaths;
    candidates << oversized;
    for (const auto &candidate : candidates) {
        PublishedCover rejected;
        QVERIFY(!store.Publish(7, "https://cdn.example/same.png", candidate, "image/png", rejected, error));
        QVERIFY(rejected.relativePath.isEmpty());
        QFile preserved(store.AbsolutePath(published.relativePath));
        QVERIFY(preserved.open(QIODevice::ReadOnly));
        QCOMPARE(preserved.readAll(), original);
    }
}

void CoverFileStoreTests::removesOnlyBoundedOrphans()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    CoverFileStore store(directory.path() + "/covers", CoverSettings{});
    QDir().mkpath(directory.path() + "/covers");
    for (const auto &name : {"keep.png", "one.png", "two.png", "three.png"}) {
        QFile file(directory.path() + "/covers/" + name);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("x");
    }

    int removed = 0;
    QString error;
    QVERIFY2(store.RemoveOrphans(QSet<QString>{"keep.png"}, 2, removed, error), qPrintable(error));
    QCOMPARE(removed, 2);
    QVERIFY(QFileInfo::exists(directory.path() + "/covers/keep.png"));
}

QTEST_MAIN(CoverFileStoreTests)
#include "CoverFileStoreTests.moc"
