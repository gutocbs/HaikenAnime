#include <QtTest>

#include "../../src/application/media/MediaTitleResolver.h"

class MediaTitleResolverTests final : public QObject {
    Q_OBJECT

private slots:
    void usesPreferredVariantAndDeterministicFallback_data();
    void usesPreferredVariantAndDeterministicFallback();
    void doesNotMutateSourceTitles();
};

void MediaTitleResolverTests::usesPreferredVariantAndDeterministicFallback_data() {
    QTest::addColumn<QString>("preference");
    QTest::addColumn<QString>("romaji");
    QTest::addColumn<QString>("english");
    QTest::addColumn<QString>("native");
    QTest::addColumn<QString>("expected");

    QTest::newRow("romaji preferred") << "romaji" << "Romaji" << "English" << "Native" << "Romaji";
    QTest::newRow("romaji to english") << "romaji" << "" << "English" << "Native" << "English";
    QTest::newRow("romaji to native") << "romaji" << "" << "" << "Native" << "Native";
    QTest::newRow("romaji empty") << "romaji" << "" << "" << "" << "";
    QTest::newRow("english preferred") << "english" << "Romaji" << "English" << "Native" << "English";
    QTest::newRow("english to romaji") << "english" << "Romaji" << "" << "Native" << "Romaji";
    QTest::newRow("english to native") << "english" << "" << "" << "Native" << "Native";
    QTest::newRow("english empty") << "english" << "" << "" << "" << "";
    QTest::newRow("native preferred") << "native" << "Romaji" << "English" << "Native" << "Native";
    QTest::newRow("native to romaji") << "native" << "Romaji" << "English" << "" << "Romaji";
    QTest::newRow("native to english") << "native" << "" << "English" << "" << "English";
    QTest::newRow("native empty") << "native" << "" << "" << "" << "";
    QTest::newRow("blank preferred ignored") << "english" << "Romaji" << "   " << "Native" << "Romaji";
    QTest::newRow("invalid preference normalizes") << "obsolete" << "Romaji" << "English" << "Native" << "Romaji";
}

void MediaTitleResolverTests::usesPreferredVariantAndDeterministicFallback() {
    QFETCH(QString, preference);
    QFETCH(QString, romaji);
    QFETCH(QString, english);
    QFETCH(QString, native);
    QFETCH(QString, expected);
    Media media;
    media.Name = romaji;
    media.EnglishName = english;
    media.OriginalName = native;
    QCOMPARE(ResolveMediaTitle(media, preference), expected);
}

void MediaTitleResolverTests::doesNotMutateSourceTitles() {
    Media media;
    media.Name = QStringLiteral("Romaji");
    media.EnglishName = QStringLiteral("English");
    media.OriginalName = QStringLiteral("Native");
    const Media before = media;
    QCOMPARE(ResolveMediaTitle(media, QStringLiteral("native")), QStringLiteral("Native"));
    QCOMPARE(media.Name, before.Name);
    QCOMPARE(media.EnglishName, before.EnglishName);
    QCOMPARE(media.OriginalName, before.OriginalName);
}

QTEST_MAIN(MediaTitleResolverTests)
#include "MediaTitleResolverTests.moc"
