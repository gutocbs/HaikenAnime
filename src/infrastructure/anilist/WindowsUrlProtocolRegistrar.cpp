#include "WindowsUrlProtocolRegistrar.h"

#include <QSettings>

#include <utility>

namespace {
bool writeRegistryValue(const WindowsUrlProtocolRegistrar::Write &write, QString &error) {
    QSettings settings(write.registryPath, QSettings::NativeFormat);
    settings.setValue(write.key, write.value);
    settings.sync();
    if (settings.status() == QSettings::NoError) {
        error.clear();
        return true;
    }
    error = QStringLiteral("Could not register the HaikenAnime URL protocol.");
    return false;
}
}

WindowsUrlProtocolRegistrar::WindowsUrlProtocolRegistrar(RegistryWriter writer)
    : writer_(writer ? std::move(writer) : writeRegistryValue) {
}

bool WindowsUrlProtocolRegistrar::registerProtocol(const QString &scheme,
                                                   const QString &applicationPath,
                                                   QString &error) const {
    error.clear();
    if (scheme.trimmed().isEmpty() || applicationPath.trimmed().isEmpty()) {
        error = QStringLiteral("A URL protocol scheme and application path are required.");
        return false;
    }

    const QString root = QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes\\%1").arg(scheme);
    const QList<Write> writes{
        {root, {}, QStringLiteral("URL:HaikenAnime Protocol")},
        {root, QStringLiteral("URL Protocol"), {}},
        {root + QStringLiteral("\\DefaultIcon"), {}, applicationPath + QStringLiteral(",0")},
        {root + QStringLiteral("\\shell\\open\\command"), {},
         QStringLiteral("\"") + applicationPath + QStringLiteral("\" \"%1\"")}
    };
    for (const auto &write : writes) {
        if (!writer_(write, error)) return false;
    }
    return true;
}
