#include <QtTest>

#include "../../src/application/media/AlternativeNames.h"

class AlternativeNamesTests final : public QObject {
    Q_OBJECT

private slots:
    void keepsExistingOrderAndAppendsNewIncoming();
    void ignoresIncomingDuplicatesIgnoringCaseAndWhitespace();
    void ignoresDuplicatesInsideIncoming();
    void ignoresBlankIncomingNames();
    void returnsExistingWhenIncomingIsEmpty();
    void returnsIncomingWhenNothingExists();
    void preservesNonLatinNames();
};

void AlternativeNamesTests::keepsExistingOrderAndAppendsNewIncoming() {
    const auto merged = MergeAlternativeNames({QStringLiteral("User B"), QStringLiteral("User A")},
                                              {QStringLiteral("Source 1"), QStringLiteral("Source 2")});
    QCOMPARE(merged, (QStringList{QStringLiteral("User B"), QStringLiteral("User A"),
                                  QStringLiteral("Source 1"), QStringLiteral("Source 2")}));
}

void AlternativeNamesTests::ignoresIncomingDuplicatesIgnoringCaseAndWhitespace() {
    const auto merged = MergeAlternativeNames({QStringLiteral("Frieren")},
                                              {QStringLiteral("  FRIEREN "), QStringLiteral("Sousou no Frieren")});
    QCOMPARE(merged, (QStringList{QStringLiteral("Frieren"), QStringLiteral("Sousou no Frieren")}));
}

void AlternativeNamesTests::ignoresDuplicatesInsideIncoming() {
    const auto merged = MergeAlternativeNames({}, {QStringLiteral("A"), QStringLiteral("a "), QStringLiteral("B")});
    QCOMPARE(merged, (QStringList{QStringLiteral("A"), QStringLiteral("B")}));
}

void AlternativeNamesTests::ignoresBlankIncomingNames() {
    const auto merged = MergeAlternativeNames({QStringLiteral("A")}, {QString(), QStringLiteral("   ")});
    QCOMPARE(merged, (QStringList{QStringLiteral("A")}));
}

void AlternativeNamesTests::returnsExistingWhenIncomingIsEmpty() {
    const QStringList existing{QStringLiteral("A"), QStringLiteral("B")};
    QCOMPARE(MergeAlternativeNames(existing, {}), existing);
}

void AlternativeNamesTests::returnsIncomingWhenNothingExists() {
    const QStringList incoming{QStringLiteral("A"), QStringLiteral("B")};
    QCOMPARE(MergeAlternativeNames({}, incoming), incoming);
}

void AlternativeNamesTests::preservesNonLatinNames() {
    const auto merged = MergeAlternativeNames({QStringLiteral("葬送のフリーレン")},
                                              {QStringLiteral("葬送のフリーレン"), QStringLiteral("フリーレン")});
    QCOMPARE(merged, (QStringList{QStringLiteral("葬送のフリーレン"), QStringLiteral("フリーレン")}));
}

QTEST_MAIN(AlternativeNamesTests)
#include "AlternativeNamesTests.moc"
