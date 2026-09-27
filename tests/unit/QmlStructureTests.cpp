#include <QFile>
#include <QRegularExpression>
#include <QtTest>

class QmlStructureTests final : public QObject {
    Q_OBJECT

private slots:
    void scanExtensionGridUsesItsAvailableWidth();
};

void QmlStructureTests::scanExtensionGridUsesItsAvailableWidth() {
    QFile file(QStringLiteral(HAIKENANIME_TEST_SOURCE_DIR "/resources/qml/SettingsScreen.qml"));
    QVERIFY2(file.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(file.errorString()));

    const QString source = QString::fromUtf8(file.readAll());
    const qsizetype sectionStart = source.indexOf(QStringLiteral("EXTENSÕES INCLUÍDAS"));
    const qsizetype sectionEnd = source.indexOf(QStringLiteral("id: scanNowButton"), sectionStart);
    QVERIFY(sectionStart >= 0);
    QVERIFY(sectionEnd > sectionStart);

    const QString extensionSection = source.mid(sectionStart, sectionEnd - sectionStart);
    QVERIFY2(extensionSection.contains(QRegularExpression(
                 QStringLiteral(R"(Grid\s*\{[\s\S]*id\s*:\s*scanExtensionGrid)"))),
             "The extension choices must use a row-major Grid positioner.");
    QVERIFY2(extensionSection.contains(QRegularExpression(
                 QStringLiteral(R"(columns\s*:\s*width\s*>=\s*580\s*\?\s*4\s*:\s*width\s*>=\s*300\s*\?\s*2\s*:\s*1)"))),
             "The grid must derive four, two, or one columns from its own width.");
    QVERIFY2(extensionSection.contains(QRegularExpression(
                 QStringLiteral(R"(readonly\s+property\s+real\s+cellWidth\s*:[^\n]*\bwidth\b[^\n]*\bcolumns\b)"))),
             "The grid must calculate a cell width from its available width and column count.");
    QVERIFY2(extensionSection.contains(QRegularExpression(
                 QStringLiteral(R"(delegate\s*:\s*CheckBox\s*\{[\s\S]*width\s*:\s*scanExtensionGrid\.cellWidth)"))),
             "Each extension checkbox must be bounded to the grid cell width.");
}

QTEST_MAIN(QmlStructureTests)
#include "QmlStructureTests.moc"
