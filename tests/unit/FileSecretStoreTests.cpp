#include <QTemporaryDir>
#include <QtTest>

#include "../../src/infrastructure/secrets/FileSecretStore.h"

class FileSecretStoreTests : public QObject {
    Q_OBJECT

private slots:
    void savesAndLoadsCredentials();
    void rejectsIncompleteFile();
};

void FileSecretStoreTests::savesAndLoadsCredentials() {
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const auto path = temporaryDirectory.filePath(QStringLiteral("secrets.txt"));
    FileSecretStore store(path);
    const AniListCredentials expected{QStringLiteral("user"), QStringLiteral("token=value")};
    QString error;

    QVERIFY(store.saveAniListCredentials(expected, error));
    QVERIFY2(error.isEmpty(), qPrintable(error));

    AniListCredentials actual;
    QVERIFY(store.loadAniListCredentials(actual, error));
    QCOMPARE(actual.username, expected.username);
    QCOMPARE(actual.token, expected.token);
}

void FileSecretStoreTests::rejectsIncompleteFile() {
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const auto path = temporaryDirectory.filePath(QStringLiteral("secrets.txt"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("username=user\n");
    file.close();

    FileSecretStore store(path);
    AniListCredentials credentials;
    QString error;
    QVERIFY(!store.loadAniListCredentials(credentials, error));
    QVERIFY(!error.isEmpty());
}

QTEST_MAIN(FileSecretStoreTests)
#include "FileSecretStoreTests.moc"
