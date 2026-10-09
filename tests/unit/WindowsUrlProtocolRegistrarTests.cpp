#include <QtTest>

#include "../../src/infrastructure/anilist/WindowsUrlProtocolRegistrar.h"

class WindowsUrlProtocolRegistrarTests final : public QObject {
    Q_OBJECT

private slots:
    void writesCurrentUserProtocolRegistration();
};

void WindowsUrlProtocolRegistrarTests::writesCurrentUserProtocolRegistration() {
    QList<WindowsUrlProtocolRegistrar::Write> writes;
    WindowsUrlProtocolRegistrar registrar(
        [&writes](const WindowsUrlProtocolRegistrar::Write &write, QString &error) {
            writes.append(write);
            error.clear();
            return true;
        });
    QString error;

    QVERIFY(registrar.registerProtocol(QStringLiteral("haikenanime"),
                                       QStringLiteral("C:/Apps/HaikenAnime.exe"), error));

    QCOMPARE(writes.size(), 4);
    QCOMPARE(writes.at(0).registryPath, QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes\\haikenanime"));
    QCOMPARE(writes.at(0).key, QStringLiteral("."));
    QCOMPARE(writes.at(1).key, QStringLiteral("URL Protocol"));
    QVERIFY(writes.at(1).value.isValid());
    QCOMPARE(writes.at(1).value.toString(), QString());
    QCOMPARE(writes.at(2).key, QStringLiteral("."));
    QCOMPARE(writes.at(3).registryPath,
             QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes\\haikenanime\\shell\\open\\command"));
    QCOMPARE(writes.at(3).key, QStringLiteral("."));
    QCOMPARE(writes.at(3).value, QStringLiteral("\"C:/Apps/HaikenAnime.exe\" \"%1\""));
}

QTEST_MAIN(WindowsUrlProtocolRegistrarTests)
#include "WindowsUrlProtocolRegistrarTests.moc"
